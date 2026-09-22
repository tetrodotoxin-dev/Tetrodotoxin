// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/process/child.hpp"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/time.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Validation;

static constexpr Count max_arguments = 32;
static constexpr S32 poll_interval_milliseconds = 10;
static constexpr U64 nanoseconds_per_millisecond = 1'000'000;

struct Pipe {
  S32 input = -1;
  S32 output = -1;
};

static auto close_descriptor(S32& descriptor) -> void {
  if (descriptor < 0) {
    return;
  }

  close(descriptor);
  descriptor = -1;
}

static auto close_pipe(Pipe& pipe) -> void {
  close_descriptor(pipe.input);
  close_descriptor(pipe.output);
}

static auto open_pipe(Pipe& pipe) -> Bool {
  Static::Vector<S32, 2> descriptors;
  S32 result = ::pipe(descriptors.get_data());
  if (result != 0) {
    return False;
  }

  pipe.input = descriptors[0];
  pipe.output = descriptors[1];
  return True;
}

static auto set_nonblocking(S32 descriptor) -> Bool {
  S32 flags = fcntl(descriptor, F_GETFL, 0);
  if (flags < 0) {
    return False;
  }

  return fcntl(descriptor, F_SETFL, flags | O_NONBLOCK) == 0;
}

static auto read_stream(S32& descriptor, Dynamic::Bytes& output) -> Bool {
  Static::Vector<U8, 4096> buffer;
  while (true) {
    ssize_t count = read(descriptor, buffer.get_data(), buffer.get_size());
    if (count > 0) {
      output.concat(View::Bytes(buffer.get_data(), Count(count)));
      continue;
    }

    if (count == 0) {
      close_descriptor(descriptor);
      return True;
    }

    if (errno == EINTR) {
      continue;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return True;
    }

    close_descriptor(descriptor);
    return False;
  }
}

static auto write_stream(S32& descriptor, View::Bytes input, Count& offset)
    -> Bool {
  while (offset < input.get_size()) {
    ssize_t count =
        write(descriptor, input.get_data() + offset, input.get_size() - offset);
    if (count > 0) {
      offset += Count(count);
      continue;
    }

    if (count < 0 && errno == EINTR) {
      continue;
    }

    if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      return True;
    }

    if (count < 0 && errno == EPIPE) {
      close_descriptor(descriptor);
      return True;
    }

    close_descriptor(descriptor);
    return False;
  }

  close_descriptor(descriptor);
  return True;
}

static auto wait_for_child(pid_t child, S32 options, S32& status) -> pid_t {
  pid_t waited;
  do {
    waited = waitpid(child, &status, options);
  } while (waited < 0 && errno == EINTR);
  return waited;
}

static auto terminate_child(pid_t child, S32& status) -> Bool {
  kill(child, SIGKILL);
  return wait_for_child(child, 0, status) == child;
}

static auto calculate_poll_timeout(Bool child_finished, U64 deadline) -> S32 {
  if (child_finished) {
    return poll_interval_milliseconds;
  }

  U64 now = Time::now().get_stamp();
  if (now >= deadline) {
    return 0;
  }

  U64 remaining = deadline - now;
  U64 milliseconds = (remaining + nanoseconds_per_millisecond - 1) /
                     nanoseconds_per_millisecond;
  if (milliseconds > U64(poll_interval_milliseconds)) {
    return poll_interval_milliseconds;
  }

  return S32(milliseconds);
}

static auto prepare_arguments(
    const Process::Request& request,
    Static::Vector<Dynamic::Bytes, max_arguments + 1>& encoded,
    Static::Vector<char*, max_arguments + 2>& arguments) -> Bool {
  if (request.executable.is_empty() ||
      request.arguments.get_size() > max_arguments) {
    return False;
  }

  encoded[0] = request.executable;
  encoded[0].append(0);
  arguments[0] = Data::cast<char>(encoded[0].get_access().get_data());
  const auto* request_argument_data = request.arguments.get_data();
  for (Count index = 0; index < request.arguments.get_size(); index++) {
    encoded[index + 1] = request_argument_data[index];
    encoded[index + 1].append(0);
    arguments[index + 1] =
        Data::cast<char>(encoded[index + 1].get_access().get_data());
  }
  arguments[request.arguments.get_size() + 1] = nullptr;
  return True;
}

auto Process::run(const Request& request) -> Observation {
  Observation observation;
  Static::Vector<Dynamic::Bytes, max_arguments + 1> encoded_arguments;
  Static::Vector<char*, max_arguments + 2> arguments;
  Pipe input_pipe;
  Pipe output_pipe;
  Pipe error_pipe;

  Bool arguments_ready =
      prepare_arguments(request, encoded_arguments, arguments);
  if (!arguments_ready) {
    observation.runner_error = "invalid child arguments"_view;
    return observation;
  }

  // Establish independent channels before a child can inherit descriptors.
  Bool input_opened = open_pipe(input_pipe);
  Bool output_opened = open_pipe(output_pipe);
  Bool error_opened = open_pipe(error_pipe);
  Bool pipes_opened = input_opened && output_opened && error_opened;
  if (!pipes_opened) {
    close_pipe(input_pipe);
    close_pipe(output_pipe);
    close_pipe(error_pipe);
    observation.runner_error = "pipe setup failed"_view;
    return observation;
  }

  pid_t child = fork();
  if (child < 0) {
    close_pipe(input_pipe);
    close_pipe(output_pipe);
    close_pipe(error_pipe);
    observation.runner_error = "fork failed"_view;
    return observation;
  }

  if (child == 0) {
    if (input_pipe.input < 0 || output_pipe.output < 0 ||
        error_pipe.output < 0) {
      close_pipe(input_pipe);
      close_pipe(output_pipe);
      close_pipe(error_pipe);
      _exit(126);
    }

    S32 input_ready = dup2(input_pipe.input, STDIN_FILENO);
    S32 output_ready = dup2(output_pipe.output, STDOUT_FILENO);
    S32 error_ready = dup2(error_pipe.output, STDERR_FILENO);

    close_pipe(input_pipe);
    close_pipe(output_pipe);
    close_pipe(error_pipe);
    if (input_ready < 0 || output_ready < 0 || error_ready < 0) {
      _exit(126);
    }

    execv(arguments[0], arguments.get_data());
    constexpr View::Bytes message = "child exec failed\n"_view;
    write(STDERR_FILENO, message.get_data(), message.get_size());
    _exit(127);
  }

  observation.launched = True;
  close_descriptor(input_pipe.input);
  close_descriptor(output_pipe.output);
  close_descriptor(error_pipe.output);

  Bool input_ready = set_nonblocking(input_pipe.output);
  Bool output_ready = set_nonblocking(output_pipe.input);
  Bool error_ready = set_nonblocking(error_pipe.input);
  Bool channels_ready = input_ready && output_ready && error_ready;
  if (!channels_ready) {
    S32 status = 0;
    terminate_child(child, status);
    close_pipe(input_pipe);
    close_pipe(output_pipe);
    close_pipe(error_pipe);
    observation.runner_error = "channel setup failed"_view;
    return observation;
  }

  struct sigaction ignored = {};
  struct sigaction previous = {};
  ignored.sa_handler = SIG_IGN;
  sigemptyset(&ignored.sa_mask);
  S32 signal_result = sigaction(SIGPIPE, &ignored, &previous);
  if (signal_result != 0) {
    S32 status = 0;
    terminate_child(child, status);
    close_pipe(input_pipe);
    close_pipe(output_pipe);
    close_pipe(error_pipe);
    observation.runner_error = "signal setup failed"_view;
    return observation;
  }

  // Pump all three channels under one monotonic process deadline.
  Count input_offset = 0;
  Bool channels_ok = True;
  Bool child_finished = False;
  S32 child_status = 0;
  U64 deadline = Time::now().get_stamp() + request.timeout_nanoseconds;
  while (!child_finished || input_pipe.output >= 0 || output_pipe.input >= 0 ||
         error_pipe.input >= 0) {
    if (!child_finished) {
      pid_t waited = wait_for_child(child, WNOHANG, child_status);
      if (waited == child) {
        child_finished = True;
        close_descriptor(input_pipe.output);
      } else if (waited < 0) {
        observation.runner_error = "child wait failed"_view;
        child_finished = terminate_child(child, child_status);
        close_descriptor(input_pipe.output);
      }
    }

    if (!child_finished && Time::now().get_stamp() >= deadline) {
      observation.timed_out = True;
      child_finished = terminate_child(child, child_status);
      close_descriptor(input_pipe.output);
    }

    Static::Vector<struct pollfd, 3> channels;
    channels[0].fd = input_pipe.output;
    channels[0].events = POLLOUT;
    channels[1].fd = output_pipe.input;
    channels[1].events = POLLIN;
    channels[2].fd = error_pipe.input;
    channels[2].events = POLLIN;
    S32 poll_timeout = calculate_poll_timeout(child_finished, deadline);
    S32 poll_result =
        poll(channels.get_data(), channels.get_size(), poll_timeout);
    if (poll_result < 0 && errno == EINTR) {
      continue;
    }

    if (poll_result < 0) {
      observation.runner_error = "channel poll failed"_view;
      if (!child_finished) {
        child_finished = terminate_child(child, child_status);
      }
      break;
    }

    if ((channels[0].revents & POLLOUT) != 0) {
      Bool input_ok =
          write_stream(input_pipe.output, request.standard_input, input_offset);
      channels_ok &= input_ok;
    }
    if ((channels[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
      close_descriptor(input_pipe.output);
    }

    if ((channels[1].revents & (POLLIN | POLLERR | POLLHUP)) != 0) {
      Bool output_ok =
          read_stream(output_pipe.input, observation.standard_output);
      channels_ok &= output_ok;
    }
    if ((channels[1].revents & POLLNVAL) != 0) {
      channels_ok = False;
      close_descriptor(output_pipe.input);
    }

    if ((channels[2].revents & (POLLIN | POLLERR | POLLHUP)) != 0) {
      Bool error_ok = read_stream(error_pipe.input, observation.standard_error);
      channels_ok &= error_ok;
    }
    if ((channels[2].revents & POLLNVAL) != 0) {
      channels_ok = False;
      close_descriptor(error_pipe.input);
    }
  }

  close_pipe(input_pipe);
  close_pipe(output_pipe);
  close_pipe(error_pipe);
  sigaction(SIGPIPE, &previous, nullptr);

  observation.standard_input = request.standard_input.slice(0, input_offset);
  if (!channels_ok && observation.runner_error.is_empty()) {
    observation.runner_error = "channel transfer failed"_view;
  }

  if (child_finished && WIFEXITED(child_status)) {
    observation.exit_status = WEXITSTATUS(child_status);
  } else if (child_finished && WIFSIGNALED(child_status)) {
    observation.exit_status = 128 + WTERMSIG(child_status);
  }

  return observation;
}

auto Process::compare(
    const Observation& observation,
    const Expectation& expectation) -> Difference {
  if (!observation.launched || !observation.runner_error.is_empty()) {
    return Difference::Launch;
  }

  if (!(observation.standard_input == expectation.standard_input)) {
    return Difference::StandardInput;
  }

  if (observation.timed_out != expectation.timed_out) {
    return Difference::Timeout;
  }

  if (!(observation.standard_output == expectation.standard_output)) {
    return Difference::StandardOutput;
  }

  if (!(observation.standard_error == expectation.standard_error)) {
    return Difference::StandardError;
  }

  if (observation.exit_status != expectation.exit_status) {
    return Difference::ExitStatus;
  }

  return Difference::None;
}
