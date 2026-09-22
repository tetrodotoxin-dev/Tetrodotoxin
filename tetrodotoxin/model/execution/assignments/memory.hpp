// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/execution/assignment.hpp"
#include "tetrodotoxin/model/execution/value.hpp"
#include "tetrodotoxin/model/type/policies/conversion.hpp"
#include "ttx/concept/domain.hpp"
#include "ttx/semantic/transport/direct.hpp"

namespace Tetrodotoxin::Model::Execution::Assignments {

// Memory is one concrete assignable destination, with a Type policy and an
// admitted region borrowed from its owner. Every write asks the Type to convert
// the current source before starting its data transfer. A dialect can layer
// private or stage authority over Assignment without changing this storage
// implementation or making the underlying primitive Type writable.
//
// The owner keeps the region, Type and supplying code alive through all uses.
// A returned Value observes the current bytes, so an earlier successful write
// does not make subsequent observations Constant.
class Memory {
 public:
  constexpr Memory(
      Ttx::Concept::Abstract type,
      Ttx::Data::Form::Storage storage)
      : type(type), storage(storage) {}

  auto get_data() const -> Perimortem::Core::View::Bytes { return {}; }
  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage output) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto assign(Ttx::Concept::Abstract value) const -> Perimortem::Utility::
      Result<Ttx::Data::Status, Ttx::Semantic::Negotiation::Binding::Failure>;

 private:
  Ttx::Concept::Abstract type;
  Ttx::Data::Form::Storage storage;
};

}  // namespace Tetrodotoxin::Model::Execution::Assignments
