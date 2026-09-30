// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/model/execution/statements/block.hpp"

#include "perimortem/core/reader/binary.hpp"
#include "perimortem/core/writer/binary.hpp"

#include "ttx/concept/answers/none.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;
using namespace Tetrodotoxin::Model::Execution::Statements;
using Tetrodotoxin::Model::Execution::Policies::Ordered;

auto Block::supports(System::Uuid id) const -> Binding::Status {
  return id == Ordered::contract_id ? Binding::Status::Satisfied
                                    : Binding::Status::Unknown;
}

auto Block::bind_interface(System::Uuid id, Ttx::Data::Form::Storage output)
    const -> Binding::Status {
  return id == Ordered::contract_id ? Binding::marker(output)
                                    : Binding::Status::Unknown;
}

auto Block::resolve_concept(Core::View::Bytes route) const -> Abstract {
  if (route.get_size() == sizeof(U64)) {
    Core::Reader::Binary<Core::Data::ByteOrder::Little> reader(route);
    const U64 index = *reader.read_u64();
    if (index < statements.get_size()) {
      return statements.get_data()[index];
    }
  }

  return Answers::None::get_none();
}

auto Block::visit_concepts(Abstract::Visitor visitor) const -> void {
  for (Count index = 0; index < statements.get_size(); ++index) {
    U8 route[sizeof(U64)];
    Core::Writer::Binary<Core::Data::ByteOrder::Little> writer(
        Core::Access::Bytes(route, sizeof(route)));
    writer << U64(index);
    visitor(
        Core::View::Bytes(route, sizeof(route)), statements.get_data()[index]);
  }
}
