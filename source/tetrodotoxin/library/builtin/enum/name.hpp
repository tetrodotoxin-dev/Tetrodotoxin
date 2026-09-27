// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"
#include "tetrodotoxin/source/layouts/addressable.hpp"
#include "tetrodotoxin/source/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::Enum {

// Name returns the authored case name for one Enumeration value.
class Name : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "get_name"_view;

  TTX_CONTRACT(Name, Language::Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Types::Enumeration& enumeration,
      const Language::Model::Type& result) -> Name&;

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

  constexpr auto get_parameters() const
      -> const Tetrodotoxin::Source::Layout& override {
    return parameters;
  }

  constexpr auto get_results() const -> const Tetrodotoxin::Source::Layout& override {
    return results;
  }

  auto fold_call(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::Option<const Language::Model::Pack&> receiver,
      const Language::Model::Pack& arguments) const
      -> Perimortem::Core::Option<Language::Model::Pack&> override;

 private:
  constexpr Name(
      Tetrodotoxin::Source::Layouts::Addressable& self,
      const Language::Types::Enumeration& enumeration,
      const Language::Model::Type& result)
      : enumeration(enumeration),
        result_type(result),
        parameters(self, 1),
        results(result, 1) {}

  const Language::Types::Enumeration& enumeration;
  const Language::Model::Type& result_type;
  Tetrodotoxin::Source::Layouts::Ranged parameters;
  Tetrodotoxin::Source::Layouts::Ranged results;

  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "Returns the authored name of this Enumeration value or an empty View."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::Enum
