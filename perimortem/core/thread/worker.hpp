// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Perimortem::Core::Thread {

class Worker {
 public:
  // The job function is called with a stable copy of the provided job data that
  // is valid for as long as the worker is alive, even if the original view is
  // invalidated by the spawning thread.
  using JobFunction = void (*)(Core::View::Bytes);

  Worker() = default;
  ~Worker();
  Worker(Worker&&);
  auto operator=(Worker&&) -> Worker&;

  // Delete copy constructors
  Worker(const Worker&) = delete;
  auto operator=(const Worker&) -> Worker& = delete;

  // Explicitly blocks execution until the worker exits.
  // Automatically called on destruction.
  auto join() -> void;

  // Starts a worker on a separate thread with a copy of the provided job data.
  // Each worker / thread has it's own working memory space.
  static auto start(
      Core::View::Bytes name,
      JobFunction job_function,
      Core::View::Bytes job_data = Core::View::Bytes()) -> Worker;

  // Checks if the thread is a main thread spawned by a non Perimortem system.
  // Unless a Foreign system creates threads Perimortem only provides one main
  // thread.
  //
  // The negative one id distinguishes the main thread from owned workers.
  static auto on_main_thread() -> Bool;

  static auto get_thread_id() -> Count;
  static auto get_thread_name() -> View::Bytes;

  // Returns the number of currently occupied worker slots.
  static auto get_worker_count() -> Count;

  static constexpr auto max_workers() -> Count { return 64; };

 private:
  // pthread_t is an unsigned long on Linux x86_64 (8 bytes, 8 byte aligned).
  // Validated by static_assert in thread.cpp.
  U64 handle = 0;
};

}  // namespace Perimortem::Core::Thread
