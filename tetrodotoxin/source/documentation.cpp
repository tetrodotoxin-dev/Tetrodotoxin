// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/documentation.hpp"

auto Tetrodotoxin::Source::Documentation::get_interface() const -> Contracts::Documentation {
  static const tetrodotoxin_source_documentation_ops operations = {
    [](const void* source) -> Count {
      return static_cast<const Documentation*>(source)->line_count();
    },
    [](const void* source, Count index) -> perimortem_view_bytes {
      const auto line =
          static_cast<const Documentation*>(source)->get_line(index);
      return {line.get_data(), line.get_size()};
    },
  };
  return Contracts::Documentation({this, &operations});
}

auto Tetrodotoxin::Source::Documentation::get_empty() -> const Documentation& {
  static constexpr Documentation documentation;
  return documentation;
}
