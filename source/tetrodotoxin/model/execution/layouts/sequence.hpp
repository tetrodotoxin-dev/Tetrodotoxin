// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/execution/layout.hpp"

namespace Tetrodotoxin::Model::Execution::Layouts {

// Sequence lends the provider's ordered fields directly. The owner chooses
// their storage and lifetime, so a source graph and a restored publication can
// use the same Layout contract without either owning the other's allocator.
class Sequence {
 public:
  constexpr explicit Sequence(
      Perimortem::Core::View::Vector<Ttx::Concept::Abstract> fields = {})
      : fields(fields) {}
  auto get_interface() const -> Execution::Layout {
    return Execution::Layout(
        {this,
         [](const void* self) -> Count {
           return static_cast<const Sequence*>(self)->fields.get_size();
         },
         [](const void* self, Count index) -> ttx_abstract {
           return static_cast<const Sequence*>(self)
               ->fields.get_data()[index]
               .get_abi();
         }});
  }

 private:
  Perimortem::Core::View::Vector<Ttx::Concept::Abstract> fields;
};

}  // namespace Tetrodotoxin::Model::Execution::Layouts
