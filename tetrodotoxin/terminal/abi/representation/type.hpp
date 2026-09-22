// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/source/layout.hpp"

namespace Tetrodotoxin::Terminal::Abi::Representation {

// Type projects the native interface shape of one completed Library Type. The
// projection stays independent from an instruction producer, letting C, C++,
// LLVM, and future native Terminals consume the same graph derived agreement.
class Type {
 public:
  enum class Kind : U8 {
    Value,
    Enumeration,
    Fixed,
    Option,
    Result,
    Range,
    View,
    Access,
    Implementation,
    Structure,
    ObjectStorage,
    Object,
    Context,
  };

  static auto get_kind(const Tetrodotoxin::Source::Type& type)
      -> Perimortem::Core::Option<Kind>;

  static auto get_width(const Tetrodotoxin::Source::Type& type)
      -> Perimortem::Core::Option<Count>;

  static auto get_element(const Tetrodotoxin::Source::Type& type)
      -> Perimortem::Core::Option<
          const Tetrodotoxin::Library::Language::Model::Type&>;

  static auto get_flag(const Tetrodotoxin::Source::Type& type)
      -> Perimortem::Core::Option<
          const Tetrodotoxin::Library::Language::Model::Type&>;

  static auto get_error(const Tetrodotoxin::Source::Type& type)
      -> Perimortem::Core::Option<
          const Tetrodotoxin::Library::Language::Model::Type&>;

  static auto get_extent(const Tetrodotoxin::Source::Type& type)
      -> Perimortem::Core::Option<Count>;

  static auto get_fields(const Tetrodotoxin::Source::Type& type)
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Layout&>;

  static auto is_real(const Tetrodotoxin::Source::Type& type) -> Bool;

  static auto is_signed(const Tetrodotoxin::Source::Type& type) -> Bool;

  static auto is_flag(const Tetrodotoxin::Source::Type& type) -> Bool;

  static auto is_object(const Tetrodotoxin::Source::Type& type) -> Bool;
};

}  // namespace Tetrodotoxin::Terminal::Abi::Representation
