// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/source/documentation.hpp"

namespace Tetrodotoxin::Source::Documentations {

// Comments exposes a borrowed ordered sequence of documentation lines.
// Source parsers use it when adjacent comment tokens should remain separate
// for proper document reconstruction.
class Block : public Tetrodotoxin::Source::Documentation {
 public:
  constexpr Block(
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> lines)
      : lines(lines) {}

  constexpr auto get_line(Count index) const
      -> Perimortem::Core::View::Bytes override {
    return index < lines.get_size() ? lines.get_data()[index]
                                    : Perimortem::Core::View::Bytes();
  }

  constexpr auto line_count() const -> Count override {
    return lines.get_size();
  }

 private:
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> lines;
};

}  // namespace Tetrodotoxin::Source::Documentations
