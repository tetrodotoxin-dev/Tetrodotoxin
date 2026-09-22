// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/thread/worker.hpp"

#include <pthread.h>
#include <sched.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

static_assert(
    sizeof(pthread_t) == sizeof(U64) && alignof(pthread_t) == sizeof(U64),
    "pthread_t layout differs from U64. Update Thread::handle");

using namespace Perimortem::Core;

// Stores the logical thread id.
// A thread created outside Perimortem uses the negative one id and represents
// the main thread for worker queries.
static thread_local U64 this_thread_id = Count(-1);
static thread_local View::Bytes this_thread_name = ""_view;

using WorkerJobFunction = Thread::Worker::JobFunction;

// ThreadInitializer owns the full worker startup transaction.
// Constructing it blocks the host thread long enough for the spawned thread to
// copy every caller owned view into its own thread local storage. That keeps
// the safe handoff automatic instead of making every caller remember the
// lifetime rules.
class ThreadInitializer {
  friend Thread::Worker;

 public:
  ThreadInitializer(
      View::Bytes requested_name,
      WorkerJobFunction job_function,
      View::Bytes job_data,
      U64& handle)
      : requested_name(requested_name),
        job_function(job_function),
        job_data(job_data),
        sink(Diagnostics::Log::get_sink()),
        disable_log_header(Diagnostics::Log::get_disable_header()) {
    if (!job_function) {
      Diagnostics::Log::fatal(
          "Thread::Worker requested to start without a job function."_view);
    }

    // Try to create a thread with the appropriate job data.
    // If for some reason the system fails to create the thread then emit fatal
    // log and exit since we are most likely in a broken state.
    thread_id = reserve_thread();
    const auto pthread_result =
        pthread_create(Data::cast<pthread_t>(&handle), nullptr, dispatch, this);
    if (pthread_result != 0) {
      release_thread(thread_id);
      Diagnostics::Log::fatal("Thread::Worker failed to create pthread."_view);
    }

    // Block until the spawned thread marks itself as initialized.
    // This blocks the host thread until all required data is safely copied.
    while (Bool(__atomic_load_n(&initialized.value, __ATOMIC_ACQUIRE)) !=
           True) {
      sched_yield();
    }
  };

  // Prevent dumb bugs from accidentally copying a thread initializer.
  ThreadInitializer(const ThreadInitializer&) = delete;
  auto operator=(const ThreadInitializer&) -> ThreadInitializer& = delete;

 private:
  // Releases the host thread and lets it clean up any data that was loaned to
  // the worker thread.
  auto mark_initialized() -> void {
    __atomic_store_n(&initialized.value, True.value, __ATOMIC_RELEASE);
  }

  // Reserves a thread slot for a worker thread.
  static auto reserve_thread() -> Count {
    pthread_mutex_lock(&mutex);

    auto worker_index = Algorithm::min_element<U8>(thread_occupancy);
    if (thread_occupancy[worker_index] != 0) {
      pthread_mutex_unlock(&mutex);
      Diagnostics::Log::fatal(
          "Program requested to create a Thread::Worker but all 64 Workers "
          "were already occupied."_view);
    }

    thread_occupancy[worker_index] = 1;
    pthread_mutex_unlock(&mutex);
    return worker_index;
  }

  static auto release_thread(Count worker_index) -> void {
    pthread_mutex_lock(&mutex);
    thread_occupancy[worker_index] = 0;
    pthread_mutex_unlock(&mutex);
  }

  static auto get_worker_count() -> Count {
    pthread_mutex_lock(&mutex);

    Count worker_count = 0;
    for (Count i = 0; i < thread_occupancy.get_size(); i++) {
      worker_count += thread_occupancy[i] != 0 ? 1 : 0;
    }

    pthread_mutex_unlock(&mutex);
    return worker_count;
  }

  static auto copy_thread_bytes(View::Bytes source_bytes) -> View::Bytes {
    // Avoid allocations if no thread data was provided.
    if (source_bytes.is_empty()) {
      return View::Bytes();
    }

    const auto allocation = Bibliotheca::check_out(source_bytes.get_size());
    Data::copy(
        allocation.ptr, source_bytes.get_data(), source_bytes.get_size());
    return View::Bytes(allocation.ptr, source_bytes.get_size());
  }

  static auto set_system_thread_name(View::Bytes thread_name) -> void {
    // Don't bother with setting the thread name if it's empty
    if (thread_name.is_empty()) {
      return;
    }

    // Max name length for pthreads is 16 (including the null terminator)
    // Static::Bytes zero extends for names shorter than 16.
    Static::Bytes<16> system_thread_name(thread_name);

    // If the name is longer than 15 characters make sure we still have a null.
    system_thread_name[15] = 0;

    pthread_setname_np(
        pthread_self(), Data::cast<char>(system_thread_name.get_data()));
  }

  static auto dispatch(void* initializer_address) -> void* {
    // To avoid race condidtions the spawning thread has already reserved a
    // valid thread id ahead of time.
    auto& initializer = *Data::cast<ThreadInitializer>(initializer_address);
    this_thread_id = initializer.thread_id;

    // Set the thread name both in engine and at the OS level to aid in
    // debugging and tracing.
    this_thread_name = copy_thread_bytes(initializer.requested_name);
    set_system_thread_name(this_thread_name);

    // Configure the thread logger based on the spawned state.
    // The thread can override this later but it's useful to at least inherit
    // the spawning threads log state by default.
    Diagnostics::Log::set_sink(initializer.sink);
    Diagnostics::Log::set_disable_header(initializer.disable_log_header);

    // Copy out the job data from the initializer before it's invalid.
    WorkerJobFunction job_function = initializer.job_function;
    View::Bytes job_data = copy_thread_bytes(initializer.job_data);

    // Log the thread start.
    {
      Diagnostics::Log::Message<128> start_message(
          Diagnostics::Log::Level::Debug);
      start_message << "Thread::Worker spawned worker_index="_view
                    << this_thread_id << " job_bytes="_view
                    << job_data.get_size();
    }

    // Run the thread job with the now thread localized job data.
    initializer.mark_initialized();
    job_function(job_data);

    // Log the thread exit before we go to release the thread.
    // If for some reason we hit a hang this can be helpful in debugging.
    {
      Diagnostics::Log::Message<128> exit_message(
          Diagnostics::Log::Level::Debug);
      exit_message << "Thread::Worker completed worker_index="_view
                   << this_thread_id;
    }

    // The job data lives in this thread's Bibliotheca. Thread exit owns
    // cleanup.
    release_thread(this_thread_id);
    return nullptr;
  }

  Count thread_id = 0;
  View::Bytes requested_name;
  WorkerJobFunction job_function;
  View::Bytes job_data;
  Diagnostics::Log::Sink sink;
  Bool disable_log_header;
  Bool initialized = False;

  static inline pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  static inline Static::Vector<U8, Thread::Worker::max_workers()>
      thread_occupancy;
};

Thread::Worker::Worker(Worker&& other_worker) : handle(other_worker.handle) {
  other_worker.handle = 0;
}

Thread::Worker::~Worker() {
  // Avoid C++'s mistakes and make sure Workers actually `join` on exit.
  join();
}

auto Thread::Worker::operator=(Worker&& other_worker) -> Worker& {
  if (this == &other_worker) {
    return *this;
  }

  join();
  handle = other_worker.handle;
  other_worker.handle = 0;
  return *this;
}

auto Thread::Worker::join() -> void {
  if (handle) {
    pthread_join(*Data::cast<pthread_t>(&handle), nullptr);
    handle = 0;
  }
}

auto Thread::Worker::start(
    Core::View::Bytes name,
    JobFunction job_function,
    Core::View::Bytes job_data) -> Thread::Worker {
  Thread::Worker worker;
  ThreadInitializer initializer(name, job_function, job_data, worker.handle);
  return worker;
}

auto Thread::Worker::on_main_thread() -> Bool {
  return this_thread_name.is_empty();
}

auto Thread::Worker::get_thread_id() -> Count {
  return Count(this_thread_id);
}

auto Thread::Worker::get_thread_name() -> View::Bytes {
  if (on_main_thread()) {
    return "main"_view;
  }

  return this_thread_name;
}

auto Thread::Worker::get_worker_count() -> Count {
  return ThreadInitializer::get_worker_count();
}
