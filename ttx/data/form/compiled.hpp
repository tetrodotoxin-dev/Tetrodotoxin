// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/bytes.hpp"

#include "ttx/data/form/compiler.hpp"
#include "ttx/data/form/representation.hpp"

namespace Ttx::Data::Form {

// A static Schema can be normalized during C++ translation so its consumers
// need no runtime compilation. Compiled publishes the same bytes as the runtime
// compiler and keeps them for the lifetime of its module. Preparation storage
// is discarded after constant evaluation.
//
// C++ needs the output array's size before constructing it. One preparation
// determines that size without writing bytes, then a second prepares and fills
// the array. Repeating preparation lets both passes use the runtime compiler's
// algorithm without a separate implementation for measuring constant schemas.
template <const auto& source>
class Compiled {
 private:
  static consteval auto measure() -> Count {
    Compiler compiler;
    const auto status = compiler.compile(source);
    return status == Status::Success ? compiler.get_size() : 0;
  }

  static constexpr Count size = measure();
  static_assert(size != 0, "Invalid constant TTX Schema");

  struct Storage {
    Perimortem::Core::Static::Bytes<size> bytes;
    Representation representation;

    consteval Storage() : representation(bytes.get_data(), size) {
      // Measurement admitted this exact immutable source and established the
      // output extent. The repeated preparation follows the same algorithm,
      // so publication needs no second failure state or allocation policy.
      Compiler compiler;
      compiler.compile(source);
      compiler.write(Perimortem::Core::Access::Bytes(bytes.get_data(), size));
    }
  };

 public:
  static constexpr auto get_representation() -> const Representation& {
    static constexpr Storage storage;
    return storage.representation;
  }
};

}  // namespace Ttx::Data::Form
