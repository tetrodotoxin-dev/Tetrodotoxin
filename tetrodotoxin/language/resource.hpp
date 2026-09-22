// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/unknown.hpp"

namespace Tetrodotoxin::Language {

// Resource exposes bytes retained by a concrete owner. The concrete owner
// keeps both contents and lifetime stable. A consumer may borrow get_value only
// when its domain cannot outlive that dependency domain. A shared domain
// satisfies that contract without another allocation.
class Resource : public Tetrodotoxin::Source::Abstract {
 public:
  TTX_CONTRACT(Resource, Tetrodotoxin::Source::Abstract);

  TTX_NAME("Resource"_view);

  TTX_EMPTY_DOCUMENTATION();

  virtual constexpr auto get_value() const -> Perimortem::Core::View::Bytes = 0;
};

}  // namespace Tetrodotoxin::Language
