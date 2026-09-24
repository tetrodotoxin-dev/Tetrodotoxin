// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/bibliotheca.hpp"

#include "validation/unit_test.hpp"

#include <errno.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace Perimortem::Core;
using namespace Validation;

static Harness Allocator = {.name = "Core::Bibliotheca"_view};

// Oversized requests must reach the fatal diagnostic before either indexing
// the archive or shifting a size past Count's width. Child processes let us
// distinguish that deliberate abort from a memory fault without allocating
// a giant slab or terminating the test runner.
PERIMORTEM_UNIT_TEST(Allocator, oversized_request) {
  const Count requests[] = {(Count(1) << 35) + 1, Count(-1)};
  for (const Count requested : requests) {
    const auto child = fork();
    ASSERT(child >= 0);
    if (child == 0) {
      const rlimit limit = {0, 0};
      setrlimit(RLIMIT_CORE, &limit);
      Bibliotheca::check_out(requested);
      _exit(0);
    }

    int status = 0;
    pid_t completed;
    do {
      completed = waitpid(child, &status, 0);
    } while (completed < 0 && errno == EINTR);

    ASSERT(completed == child);
    EXPECT(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
  }
}
