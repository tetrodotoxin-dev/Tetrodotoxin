// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Tetrodotoxin::Package::Archive {

// One completed semantic member carried by a Package Archive. The member name
// is the deterministic first route discovered from the Package root; it keys
// graph restoration without becoming an intrinsic Monograph name. The Dialect
// name selects the owner that can interpret its exact opaque payload.
//
// Member retains those three facts without interpreting or versioning the
// payload itself.
class Member {
 public:
  constexpr Member(
      Perimortem::Core::View::Bytes semantic_name,
      Perimortem::Core::View::Bytes dialect_name,
      Perimortem::Core::View::Bytes payload)
      : semantic_name(semantic_name),
        dialect_name(dialect_name),
        payload(payload) {}

  constexpr auto get_semantic_name() const -> Perimortem::Core::View::Bytes {
    return semantic_name;
  }

  constexpr auto get_dialect_name() const -> Perimortem::Core::View::Bytes {
    return dialect_name;
  }

  constexpr auto get_payload() const -> Perimortem::Core::View::Bytes {
    return payload;
  }

 private:
  Perimortem::Core::View::Bytes semantic_name;
  Perimortem::Core::View::Bytes dialect_name;
  Perimortem::Core::View::Bytes payload;
};

}  // namespace Tetrodotoxin::Package::Archive
