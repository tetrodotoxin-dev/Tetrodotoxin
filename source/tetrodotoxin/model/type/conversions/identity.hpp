// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/type/policies/conversion.hpp"
#include "tetrodotoxin/model/type/storage.hpp"
#include "ttx/concept/domain.hpp"

namespace Tetrodotoxin::Model::Type::Conversions {

// This receiving policy admits an unchanged observation when its required
// semantic property and complete storage form both hold. It is useful for
// native scalars and plain records. It does not claim that the two Types are
// equivalent, nor does it authorize mutation of either subject. Every call
// observes Domain and the encountered Type policies again, without a cached
// admission decision based on an earlier source Type.
class Identity {
 public:
  constexpr Identity(
      Perimortem::System::Uuid policy,
      const Ttx::Data::Form::Representation& representation)
      : policy(policy), representation(representation) {}

  auto convert(Ttx::Concept::Abstract value) const
      -> Perimortem::Utility::Result<
          Ttx::Concept::Abstract,
          Ttx::Semantic::Negotiation::Binding::Failure>;
  auto get_interface() const -> Policies::Conversion;

 private:
  Perimortem::System::Uuid policy;
  const Ttx::Data::Form::Representation& representation;
};

}  // namespace Tetrodotoxin::Model::Type::Conversions
