// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/system/file.hpp"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef PERI_LINUX
#include <linux/openat2.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/system/path.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;

// Bibliotheca can represent byte allocations through the 32 GiB archive.
// Reject the next radix before it can index beyond that owned range.
static constexpr Count max_read_size = Count(1) << 35;

// Max path size for now. Used for storing root operations across threads
// without needing to handshake dynamic memory allocations.
static constexpr Count max_path_size = 512;
static constexpr Count max_warning_size = max_path_size + 256;

// Operation identities are part of the diagnostic contract. Every stage of
// one filesystem transaction uses the same value so its messages cannot drift.
static constexpr View::Bytes file_read_operation = "System::File read"_view;
static constexpr View::Bytes file_write_operation = "System::File write"_view;
static constexpr View::Bytes file_replace_operation =
    "System::File replace"_view;
static constexpr View::Bytes root_write_operation =
    "System::File::Root write"_view;
static constexpr View::Bytes root_remove_operation =
    "System::File::Root remove"_view;
static constexpr View::Bytes root_exists_operation =
    "System::File::Root exists"_view;
static constexpr View::Bytes root_close_operation =
    "System::File::Root close"_view;

enum class FailureReporting {
  Silent,
  Warning,
};

// File operations prepare one bounded native path, open the selected object
// once, perform all metadata and content work against that opened object, then
// include closure in the result. Root operations retain a directory descriptor
// so member paths never need to reopen the authored root location.

// Keeps diagnostic storage bounded while preserving the operation, path, and
// exact transaction stage that explain the failure.
static auto log_file_warning(
    View::Bytes operation,
    View::Bytes path,
    View::Bytes stage,
    View::Bytes detail_name,
    S64 detail) -> void {
  Count logged_path_size = path.get_size();
  if (logged_path_size != 0 && path[logged_path_size - 1] == '\0') {
    logged_path_size--;
  }

  if (logged_path_size > max_path_size) {
    logged_path_size = max_path_size;
  }

  View::Bytes logged_path(path.get_data(), logged_path_size);
  Diagnostics::Log::Message<max_warning_size> warning(
      Diagnostics::Log::Level::Warning, Diagnostics::Source());
  warning << operation << " failed. path="_view << logged_path << " stage="_view
          << stage << ' ' << detail_name << '=' << detail;
}

static auto report_file_warning(
    FailureReporting reporting,
    View::Bytes operation,
    View::Bytes path,
    View::Bytes stage,
    View::Bytes detail_name,
    S64 detail) -> void {
  if (reporting == FailureReporting::Warning) {
    log_file_warning(operation, path, stage, detail_name, detail);
  }
}

// Classifies and sizes the same opened object that will provide the content.
// Reading metadata through its descriptor prevents a pathname replacement from
// changing which object the transaction observes.
static auto get_file_fingerprint(
    S32 descriptor,
    View::Bytes operation,
    View::Bytes path,
    FailureReporting reporting) -> Option<File::Fingerprint> {
  struct stat64 status;
  S32 status_read = fstat64(descriptor, &status);
  if (status_read != 0) {
    S32 status_error = errno;
    report_file_warning(
        reporting, operation, path, "metadata"_view, "errno"_view,
        status_error);
    return {};
  }

  if (!S_ISREG(status.st_mode)) {
    report_file_warning(
        reporting, operation, path, "classification"_view, "mode"_view,
        S64(status.st_mode));
    return {};
  }

  if (status.st_size < 0) {
    report_file_warning(
        reporting, operation, path, "size"_view, "value"_view,
        S64(status.st_size));
    return {};
  }

  U64 size = U64(status.st_size);
  if (size > max_read_size) {
    report_file_warning(
        reporting, operation, path, "size"_view, "value"_view, S64(size));
    return {};
  }

  return File::Fingerprint(
      U64(status.st_dev), U64(status.st_ino), size, S64(status.st_mtim.tv_sec),
      S64(status.st_mtim.tv_nsec), S64(status.st_ctim.tv_sec),
      S64(status.st_ctim.tv_nsec));
}

// Fills either a Dynamic or Managed Bytes by resizing it to the valid size and
// then reading the file directly into it's memory buffer. For arena's this has
// the benefit of saving a copy.
template <typename bytes_type>
static auto read_file(
    FILE* file,
    bytes_type& data,
    File::Fingerprint& fingerprint,
    View::Bytes operation,
    View::Bytes path,
    FailureReporting reporting) -> Bool {
  auto selected =
      get_file_fingerprint(fileno(file), operation, path, reporting);
  if (!selected) {
    return False;
  }

  fingerprint = *selected;
  Count size = selected->get_size();
  data.resize(size);
  if (size == 0) {
    return True;
  }

  CppSize items_read =
      fread(data.get_access().get_data(), 1, CppSize(size), file);
  if (items_read != CppSize(size) || ferror(file) != 0) {
    S32 read_error = errno;
    report_file_warning(
        reporting, operation, path, "content"_view, "errno"_view,
        read_error);
    return False;
  }

  return True;
}

// Completes the content stage against an already opened stream. Empty content
// succeeds because the selected open mode already established truncation.
static auto write_file(
    FILE* file,
    View::Bytes data,
    View::Bytes operation,
    View::Bytes path) -> Bool {
  if (data.is_empty()) {
    return True;
  }

  CppSize items_written = fwrite(data.get_data(), data.get_size(), 1, file);
  if (items_written != 1) {
    S32 write_error = errno;
    log_file_warning(
        operation, path, "content"_view, "errno"_view, write_error);
    return False;
  }

  return True;
}

// Closure is part of a file transaction because buffered input or output can
// still report a failure while the stream is being released.
static auto close_stream(
    FILE* file,
    View::Bytes operation,
    View::Bytes path,
    FailureReporting reporting) -> Bool {
  S32 closed = fclose(file);
  if (closed == 0) {
    return True;
  }

  S32 close_error = errno;
  report_file_warning(
      reporting, operation, path, "close"_view, "errno"_view, close_error);
  return False;
}

#ifdef PERI_LINUX
// Descriptor only operations use the same checked closure rule as streams.
static auto close_descriptor(
    S32 descriptor,
    View::Bytes operation,
    View::Bytes path,
    FailureReporting reporting) -> Bool {
  S32 closed = close(descriptor);
  if (closed == 0) {
    return True;
  }

  S32 close_error = errno;
  report_file_warning(
      reporting, operation, path, "close"_view, "errno"_view, close_error);
  return False;
}

// Root destruction cannot return a failure. Preserve the descriptor identity
// in diagnostics because Root intentionally does not retain its authored path.
static auto close_root_descriptor(S32 descriptor) -> void {
  S32 closed = close(descriptor);
  if (closed == 0) {
    return;
  }

  S32 close_error = errno;
  Diagnostics::Log::Message<192> warning(
      Diagnostics::Log::Level::Warning, Diagnostics::Source());
  warning << root_close_operation << " failed. descriptor="_view << descriptor
          << " errno="_view << close_error;
}

// Opens one member relative to the retained root descriptor. Kernel resolution
// keeps traversal beneath that root and rejects magic link escapes without
// reopening the root pathname.
static auto
    open_root_member(S32 descriptor, const char* path, U64 flags, U64 mode = 0)
        -> S32 {
  open_how policy = {
    .flags = flags,
    .mode = mode,
    .resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS,
  };
  return S32(syscall(SYS_openat2, descriptor, path, &policy, sizeof(policy)));
}
#endif

// Writes one slash normalized path and exactly one final null into caller
// storage. The returned view excludes the terminator so it retains ordinary
// byte semantics.
static auto create_path(Access::Bytes output, View::Bytes path)
    -> Option<View::Bytes> {
  Count content_size = path.get_size();
  if (content_size != 0 && path[content_size - 1] == '\0') {
    content_size--;
  }

  for (Count i = 0; i < content_size; i++) {
    if (path[i] == '\0') {
      return {};
    }
  }

  if (content_size >= output.get_size()) {
    return {};
  }

  auto* output_data = output.get_data();
  for (Count i = 0; i < content_size; i++) {
    output_data[i] = path[i] == '\\' ? '/' : path[i];
  }

  output_data[content_size] = '\0';
  return View::Bytes(output_data, content_size);
}

// Converts a root member spelling into the canonical relative form accepted by
// confined descriptor operations. Kernel resolution remains the final defense
// against links and traversal that escape the retained root.
static auto create_relative_path(Access::Bytes output, View::Bytes route)
    -> Option<View::Bytes> {
  auto native_route = create_path(output, route);
  if (!native_route || (*native_route).is_empty() ||
      (*native_route)[0] == '/') {
    return {};
  }

  Path normalized(*native_route);
  View::Bytes normalized_route = normalized.get_view();
  if (normalized_route.is_empty() || normalized.is_rooted()) {
    return {};
  }

  return create_path(output, normalized_route);
}

template <typename bytes_type>
static auto read_root_member(
    S32 descriptor,
    View::Bytes relative_path,
    bytes_type& data,
    File::Fingerprint& fingerprint) -> Bool {
  // Stage 1: Produce the bounded relative spelling used by kernel resolution.
  // Empty, rooted, and malformed routes never reach open; the requesting owner
  // retains the authored path needed to diagnose that failed probe.
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_relative_path(path_buffer, relative_path);
  if (!path) {
    return False;
  }

  const char* native_path = Data::cast<const char>((*path).get_data());

#ifdef PERI_LINUX
  // Stage 2: Open the member beneath the retained root capability, then attach
  // a stream to that exact descriptor. No path lookup occurs between them.
  S32 member =
      open_root_member(descriptor, native_path, U64(O_RDONLY | O_CLOEXEC));
  if (member < 0) {
    return False;
  }

  FILE* file = fdopen(member, "rb");
  if (!file) {
    close_descriptor(
        member, {}, relative_path, FailureReporting::Silent);
    return False;
  }

  // Stage 3: Classify and fill the selected byte owner from the same stream.
  // Checked closure completes the transaction even when content already read.
  Bool read = read_file(
      file, data, fingerprint, {}, relative_path, FailureReporting::Silent);
  Bool closed = close_stream(
      file, {}, relative_path, FailureReporting::Silent);

  return read && closed;
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}

template <typename bytes_type>
static auto read_file(View::Bytes location, bytes_type& data) -> Bool {
  // Stage 1: Serialize the caller path into bounded native storage before any
  // filesystem operation begins.
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_path(path_buffer, location);
  if (!path) {
    log_file_warning(
        file_read_operation, location, "path"_view, "size"_view,
        S64(location.get_size()));
    return False;
  }

  // Stage 2: Open the selected path exactly once. Metadata, content, and close
  // below all describe this stream even if the pathname later changes.
  const char* native_path = Data::cast<const char>((*path).get_data());
  FILE* file = fopen(native_path, "rb");
  if (!file) {
    S32 open_error = errno;
    log_file_warning(
        file_read_operation, location, "open"_view, "errno"_view, open_error);
    return False;
  }

  // Stage 3: Fill the storage selected by the public overload and include
  // stream closure in the reported result.
  File::Fingerprint fingerprint;
  Bool read = read_file(
      file, data, fingerprint, file_read_operation, location,
      FailureReporting::Warning);
  Bool closed = close_stream(
      file, file_read_operation, location, FailureReporting::Warning);

  return read && closed;
}

File::Root::Root(S32 descriptor) : descriptor(descriptor) {}

File::Root::Root(Root&& source) : descriptor(source.descriptor) {
  // Root uniquely owns the retained directory capability. Disable the source
  // immediately so only the destination can close it.
  source.descriptor = -1;
}

auto File::Root::operator=(Root&& source) -> Root& {
  if (this == &source) {
    return *this;
  }

  // Release the current capability before adopting the source descriptor.
#ifdef PERI_LINUX
  if (descriptor >= 0) {
    close_root_descriptor(descriptor);
  }
#else
#error Perimortem does not have a file implementation for this platform.
#endif

  descriptor = source.descriptor;
  source.descriptor = -1;
  return *this;
}

File::Root::~Root() {
#ifdef PERI_LINUX
  if (descriptor < 0) {
    return;
  }

  // Root has no failure channel during destruction, so closure reports through
  // the diagnostic path reserved for retained descriptors.
  close_root_descriptor(descriptor);
  descriptor = -1;
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}

auto File::Root::open(View::Bytes location) -> Option<Root> {
  // Stage 1: Materialize one bounded root path. Root does not retain this
  // spelling after the directory is opened.
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_path(path_buffer, location);
  if (!path || (*path).is_empty()) {
    return {};
  }

  const char* native_path = Data::cast<const char>((*path).get_data());

#ifdef PERI_LINUX
  // Stage 2: Retain the directory itself as the capability used by every
  // future member operation.
  S32 descriptor = ::open(native_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (descriptor < 0) {
    return {};
  }

  return Option<Root>(Root(descriptor));
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}

auto File::Root::read(View::Bytes relative_path) const
    -> Option<Dynamic::Bytes> {
  // Dynamic storage gives this overload an independently owned result.
  Dynamic::Bytes data;
  File::Fingerprint fingerprint;
  Bool read = read_root_member(descriptor, relative_path, data, fingerprint);
  if (!read) {
    return {};
  }

  return Option<Dynamic::Bytes>(static_cast<Dynamic::Bytes&&>(data));
}

auto File::Root::read_snapshot(View::Bytes relative_path) const
    -> Option<File::Snapshot> {
  Dynamic::Bytes data;
  File::Fingerprint fingerprint;
  Bool read = read_root_member(descriptor, relative_path, data, fingerprint);
  if (!read) {
    return {};
  }

  return File::Snapshot(Data::take(data), fingerprint);
}

auto File::Root::fingerprint(View::Bytes relative_path) const
    -> Option<File::Fingerprint> {
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_relative_path(path_buffer, relative_path);
  if (!path) {
    return {};
  }

  const char* native_path = Data::cast<const char>((*path).get_data());

#ifdef PERI_LINUX
  S32 member =
      open_root_member(descriptor, native_path, U64(O_RDONLY | O_CLOEXEC));
  if (member < 0) {
    return {};
  }

  auto selected = get_file_fingerprint(
      member, {}, relative_path, FailureReporting::Silent);
  Bool closed = close_descriptor(
      member, {}, relative_path, FailureReporting::Silent);

  return closed ? selected : Option<File::Fingerprint>();
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}

auto File::Root::read(Allocator::Arena& arena, View::Bytes relative_path) const
    -> Option<View::Bytes> {
  // Arena storage makes the returned view stable for the caller's Arena
  // lifetime while using the same file transaction as the Dynamic overload.
  Managed::Bytes data(arena);
  File::Fingerprint fingerprint;
  Bool read = read_root_member(descriptor, relative_path, data, fingerprint);
  if (!read) {
    return {};
  }

  return Option<View::Bytes>(data.get_view());
}

auto File::Root::write(View::Bytes data, View::Bytes relative_path) const
    -> Bool {
  // Stage 1: Reject any route that cannot be expressed as a confined relative
  // member before asking the kernel to resolve it.
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_relative_path(path_buffer, relative_path);
  if (!path) {
    log_file_warning(
        root_write_operation, relative_path, "path"_view, "size"_view,
        S64(relative_path.get_size()));
    return False;
  }

  const char* native_path = Data::cast<const char>((*path).get_data());

#ifdef PERI_LINUX
  // Stage 2: Create or replace the member beneath the retained root and attach
  // a stream to that exact descriptor.
  constexpr U64 create_mode =
      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  S32 member = open_root_member(
      descriptor, native_path, U64(O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC),
      create_mode);
  if (member < 0) {
    S32 open_error = errno;
    log_file_warning(
        root_write_operation, relative_path, "open"_view, "errno"_view,
        open_error);
    return False;
  }

  FILE* file = fdopen(member, "wb");
  if (!file) {
    S32 stream_error = errno;
    log_file_warning(
        root_write_operation, relative_path, "stream"_view, "errno"_view,
        stream_error);
    close_descriptor(
        member, root_write_operation, relative_path,
        FailureReporting::Warning);
    return False;
  }

  // Stage 3: Write the complete caller view and include stream closure in the
  // transaction result.
  Bool written = write_file(file, data, root_write_operation, relative_path);
  Bool closed = close_stream(
      file, root_write_operation, relative_path, FailureReporting::Warning);

  return written && closed;
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}

auto File::Root::remove(View::Bytes relative_path) const -> Bool {
  // Stage 1: Normalize the confined route and locate its final member name.
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_relative_path(path_buffer, relative_path);
  if (!path) {
    return False;
  }

  Count member_offset = 0;
  for (Count i = 0; i < (*path).get_size(); i++) {
    if ((*path)[i] == '/') {
      member_offset = i + 1;
    }
  }

  const char* native_path = Data::cast<const char>((*path).get_data());

#ifdef PERI_LINUX
  // Stage 2: For a nested route, retain its containing directory beneath the
  // root. The final unlink is then relative to an already resolved parent.
  S32 parent = descriptor;
  Bool close_parent = False;
  if (member_offset != 0) {
    path_buffer[member_offset - 1] = '\0';
    parent = open_root_member(
        descriptor, native_path, U64(O_PATH | O_DIRECTORY | O_CLOEXEC));
    if (parent < 0) {
      return False;
    }

    close_parent = True;
  }

  // Stage 3: Remove only the final member and close any temporary parent
  // capability before reporting success.
  const char* member_path = native_path + member_offset;
  S32 removed = unlinkat(parent, member_path, 0);

  Bool parent_closed = True;
  if (close_parent) {
    parent_closed =
        close_descriptor(
            parent, root_remove_operation, relative_path,
            FailureReporting::Warning);
  }

  return removed == 0 && parent_closed;
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}

auto File::Root::exists(View::Bytes relative_path) const -> Bool {
  // Stage 1: Reject malformed or rooted input before confined resolution.
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_relative_path(path_buffer, relative_path);
  if (!path) {
    return False;
  }

  const char* native_path = Data::cast<const char>((*path).get_data());

#ifdef PERI_LINUX
  // Stage 2: Pin the member without opening its content, classify that same
  // descriptor, then include closure in the answer.
  S32 member =
      open_root_member(descriptor, native_path, U64(O_PATH | O_CLOEXEC));
  if (member < 0) {
    return False;
  }

  struct stat64 status;
  S32 status_read = fstat64(member, &status);
  Bool regular = status_read == 0 && S_ISREG(status.st_mode);

  Bool closed = close_descriptor(
      member, root_exists_operation, relative_path,
      FailureReporting::Warning);

  return regular && closed;
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}

auto File::read(View::Bytes location) -> Option<Dynamic::Bytes> {
  // Dynamic storage gives this overload an independently owned result.
  Dynamic::Bytes data;
  Bool read = read_file(location, data);
  if (!read) {
    return {};
  }

  return Option<Dynamic::Bytes>(static_cast<Dynamic::Bytes&&>(data));
}

auto File::read(Allocator::Arena& arena, View::Bytes location)
    -> Option<View::Bytes> {
  // Arena storage returns stable bytes without introducing another owner.
  Managed::Bytes data(arena);
  Bool read = read_file(location, data);
  if (!read) {
    return {};
  }

  return Option<View::Bytes>(data.get_view());
}

auto File::write(View::Bytes data, View::Bytes location) -> Bool {
  // Stage 1: Serialize the caller path into bounded native storage.
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_path(path_buffer, location);
  if (!path) {
    log_file_warning(
        file_write_operation, location, "path"_view, "size"_view,
        S64(location.get_size()));
    return False;
  }

  const char* native_path = Data::cast<const char>((*path).get_data());

  // Stage 2: Open once for replacement, write through that stream, then treat
  // closure as part of the operation result.
  FILE* file = fopen(native_path, "wb");
  if (!file) {
    S32 open_error = errno;
    log_file_warning(
        file_write_operation, location, "open"_view, "errno"_view, open_error);
    return False;
  }

  Bool written = write_file(file, data, file_write_operation, location);
  Bool closed = close_stream(
      file, file_write_operation, location, FailureReporting::Warning);

  return written && closed;
}

auto File::replace(View::Bytes source, View::Bytes destination) -> Bool {
  Static::Bytes<max_path_size> source_buffer;
  Static::Bytes<max_path_size> destination_buffer;
  auto source_path = create_path(source_buffer, source);
  auto destination_path = create_path(destination_buffer, destination);
  if (!source_path || !destination_path) {
    View::Bytes failed = source_path ? destination : source;
    log_file_warning(
        file_replace_operation, failed, "path"_view, "size"_view,
        S64(failed.get_size()));
    return False;
  }

  const char* native_source = Data::cast<const char>(source_path->get_data());
  const char* native_destination =
      Data::cast<const char>(destination_path->get_data());
  S32 replaced = rename(native_source, native_destination);
  if (replaced == 0) {
    return True;
  }

  S32 replace_error = errno;
  log_file_warning(
      file_replace_operation, destination, "rename"_view, "errno"_view,
      replace_error);
  return False;
}

auto File::remove(View::Bytes location) -> Bool {
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_path(path_buffer, location);
  if (!path) {
    return False;
  }

  const char* native_path = Data::cast<const char>((*path).get_data());
  S32 removed = ::remove(native_path);
  return removed == 0;
}

auto File::exists(View::Bytes location) -> Bool {
  Static::Bytes<max_path_size> path_buffer;
  auto path = create_path(path_buffer, location);
  if (!path) {
    return False;
  }

  const char* native_path = Data::cast<const char>((*path).get_data());

#ifdef PERI_LINUX
  struct stat64 status;
  S32 status_read = stat64(native_path, &status);
  return status_read == 0 && Bool(status.st_mode & S_IFREG);
#else
#error Perimortem does not have a file implementation for this platform.
#endif
}
