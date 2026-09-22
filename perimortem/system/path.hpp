// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

namespace Perimortem::System {

// Stores one lexical path with normalized separators.
//
// Filesystem canonicalization requires an opened root and can race with later
// path use. Path stays lexical so callers can normalize stable cache keys
// before a capability owner performs any physical lookup.
class Path {
 public:
  static constexpr Count max_size = 510;

  Path() = default;
  Path(Core::View::Bytes path);
  Path(Core::View::Bytes base_file_path, Core::View::Bytes relative_path);

  // The Arena overload writes the accepted spelling directly into its final
  // lifetime domain so retaining owners do not proxy a temporary Path. It uses
  // `/` for one platform independent cache identity and rejects input whose
  // lexical meaning cannot be preserved exactly.
  static auto normalize(Memory::Allocator::Arena& arena, Core::View::Bytes path)
      -> Core::Option<Core::View::Bytes>;

  constexpr auto get_view() const -> Core::View::Bytes {
    return text.slice(0, size);
  }

  constexpr auto get_file() const -> Core::View::Bytes {
    Core::View::Bytes path = get_view();
    Count file_start = 0;
    for (Count i = 0; i < path.get_size(); i++) {
      if (path[i] == '/') {
        file_start = i + 1;
      }
    }

    return path.slice(file_start);
  }

  constexpr auto get_directory() const -> Core::View::Bytes {
    Core::View::Bytes path = get_view();
    Core::View::Bytes file = get_file();
    if (file.get_size() == path.get_size()) {
      return Core::View::Bytes();
    }

    Count directory_size = path.get_size() - file.get_size() - 1;
    return directory_size == 0 && is_rooted() ? path.slice(0, 1)
                                              : path.slice(0, directory_size);
  }

  constexpr auto get_extension() const -> Core::View::Bytes {
    Core::View::Bytes file = get_file();
    for (Count i = file.get_size(); i > 0; i--) {
      if (file[i - 1] == '.' && i - 1 > 0 && i < file.get_size()) {
        return file.slice(i - 1);
      }
    }

    return Core::View::Bytes();
  }

  constexpr auto is_rooted() const -> Bool {
    return !get_view().is_empty() && text[0] == '/';
  }

  constexpr operator Core::View::Bytes() const { return get_view(); }

  constexpr auto operator==(const Path& rhs) const -> Bool {
    return get_view() == rhs.get_view();
  }

 private:
  Core::Static::Bytes<max_size> text;
  Count size = 0;
};

}  // namespace Perimortem::System
