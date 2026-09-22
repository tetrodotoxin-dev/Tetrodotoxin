// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/system/path.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;

// A Path is bounded to a small fixed size, so rescanning the normalized prefix
// costs less state than retaining a second stack of segment offsets. The root
// marker is the only prefix that a parent segment may not remove.
static auto pop_segment(Access::Bytes text, Count& size) -> Bool {
  auto* data = text.get_data();
  if (size == 0 || (size == 1 && data[0] == '/')) {
    return False;
  }

  Count segment_start = data[0] == '/' ? Count(1) : Count(0);
  for (Count i = segment_start; i < size; i++) {
    if (data[i] == '/') {
      segment_start = i;
    }
  }

  size = segment_start;
  return True;
}

// Truncation could make two different paths compare equal. Reject the complete
// path when a segment does not fit instead of publishing a shortened spelling.
static auto append_segment(Access::Bytes text, Count& size, View::Bytes segment)
    -> Bool {
  auto* data = text.get_data();
  const Bool needs_separator = size != 0 && !(size == 1 && data[0] == '/');
  const Count required_size =
      segment.get_size() + (needs_separator ? Count(1) : Count(0));
  if (required_size > text.get_size() - size) {
    return False;
  }

  if (needs_separator) {
    data[size++] = '/';
  }

  Data::copy(data + size, segment.get_data(), segment.get_size());
  size += segment.get_size();
  return True;
}

// Both fixed Path values and Arena results use this transaction so their
// accepted spellings cannot drift apart.
static auto append_path(Access::Bytes text, Count& size, View::Bytes path)
    -> Bool {
  // A later system call would treat NUL as the end of a C string. Rejecting it
  // here prevents the lexical key from naming more bytes than the physical
  // lookup can observe.
  for (Count i = 0; i < path.get_size(); i++) {
    if (path[i] == '\0') {
      return False;
    }
  }

  Count i = 0;
  while (i < path.get_size()) {
    // Collapsing both separator spellings gives caches one key regardless of
    // the platform spelling used by the caller.
    while (i < path.get_size() && (path[i] == '/' || path[i] == '\\')) {
      i++;
    }

    const Count start = i;
    while (i < path.get_size() && path[i] != '/' && path[i] != '\\') {
      i++;
    }

    const View::Bytes segment = path.slice(start, i - start);
    // Empty and current directory segments are aliases of the current prefix.
    // Dropping them keeps equality independent of redundant separators.
    if (segment.is_empty() || (segment.get_size() == 1 && segment[0] == '.')) {
      continue;
    }

    // An unmatched parent would escape the lexical root selected by the caller.
    // Reject it rather than retaining a route whose confinement depends on a
    // later working directory.
    if (segment.get_size() == 2 && segment[0] == '.' && segment[1] == '.') {
      Bool popped = pop_segment(text, size);
      if (!popped) {
        return False;
      }

      continue;
    }

    Bool appended = append_segment(text, size, segment);
    if (!appended) {
      return False;
    }
  }

  return True;
}

// The writable view describes capacity rather than result length. Returning
// the final size lets the same routine serve fixed storage and exact Arena
// storage without introducing another Path representation.
static auto normalize_path(Access::Bytes text, View::Bytes path)
    -> Option<Count> {
  if (path.is_empty()) {
    return {};
  }

  Count size = 0;
  auto* data = text.get_data();
  if (path[0] == '/' || path[0] == '\\') {
    data[size++] = '/';
  }

  Bool appended = append_path(text, size, path);
  if (!appended || size == 0) {
    return {};
  }

  return size;
}

Path::Path(View::Bytes path) {
  // Stack storage keeps temporary normalization and cache probes allocation
  // free. Owners that retain the result use the Arena overload instead.
  auto normalized = normalize_path(text, path);
  if (normalized) {
    size = *normalized;
  }
}

Path::Path(View::Bytes base_file_path, View::Bytes relative_path) {
  // A rooted second path carries its own origin, so joining it to the base
  // would produce a spelling with two conflicting roots.
  const Bool rooted = !relative_path.is_empty() &&
                      (relative_path[0] == '/' || relative_path[0] == '\\');
  const Bool base_rooted =
      !base_file_path.is_empty() &&
      (base_file_path[0] == '/' || base_file_path[0] == '\\');
  if (rooted || base_rooted) {
    text[size++] = '/';
  }

  if (!rooted) {
    // The first argument names a file. Including its last segment would treat
    // the filename as a directory during relative resolution.
    Count directory_size = 0;
    for (Count i = 0; i < base_file_path.get_size(); i++) {
      if (base_file_path[i] == '/' || base_file_path[i] == '\\') {
        directory_size = i;
      }
    }

    Bool base_appended =
        append_path(text, size, base_file_path.slice(0, directory_size));
    if (!base_appended) {
      size = 0;
      return;
    }
  }

  Bool relative_appended = append_path(text, size, relative_path);
  if (!relative_appended || size == 0) {
    size = 0;
  }
}

auto Path::normalize(Allocator::Arena& arena, View::Bytes path)
    -> Option<View::Bytes> {
  // Arena cannot roll back one bump allocation. The fixed Path pass rejects
  // invalid input and measures the exact allocation before Arena state changes.
  Path validated(path);
  View::Bytes validated_text = validated.get_view();
  if (validated_text.is_empty()) {
    return {};
  }

  // A second bounded lexical pass writes into the final allocation. This avoids
  // reserving max_size for every path and avoids copying a temporary Path into
  // the Arena after normalization.
  Access::Bytes normalized_text = arena.allocate(validated_text.get_size());
  auto normalized_size = normalize_path(normalized_text, path);
  if (!normalized_size || *normalized_size != normalized_text.get_size()) {
    return {};
  }

  return normalized_text.get_view();
}
