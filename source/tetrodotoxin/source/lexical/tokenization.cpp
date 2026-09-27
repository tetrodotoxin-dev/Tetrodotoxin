// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/lexical/tokenization.hpp"

#include "tetrodotoxin/source/contents/memory.hpp"
#include "tetrodotoxin/source/lexical/cursors/stream.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"
#include "ttx/semantic/flows/copy.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Data::Form;
using namespace Ttx::Semantic::Negotiation;
using namespace Ttx::Semantic::Transport;

static auto status(Flow::Status value) -> Binding::Status {
  switch (value) {
  case Flow::Status::Success:
    return Binding::Status::Satisfied;
  case Flow::Status::Unsupported:
    return Binding::Status::Unsupported;
  case Flow::Status::BindingPending:
    return Binding::Status::Pending;
  case Flow::Status::Invalid:
  case Flow::Status::Bounds:
  case Flow::Status::Overflow:
  case Flow::Status::Incompatible:
  case Flow::Status::Denied:
  case Flow::Status::Busy:
  case Flow::Status::IoError:
  case Flow::Status::Rejected:
    return Binding::Status::Rejected;
  }

  return Binding::Status::Rejected;
}

// Publication releases this owner after the last token or spelling borrow.
// Flow retains an acquired Shared lifetime when one supplied the bytes. It is
// destroyed before the Arena containing its required representation.
class LexicalPublication {
 public:
  explicit LexicalPublication(Abstract source) : source(source) {}

  auto get_data() const -> Core::View::Bytes { return {}; }

  auto read(Source::Content input) -> Binding::Status {
    const Count size = input.get_size();
    const auto schema = Schema::range(Native<U8>::reference, size, 1, size, 1);
    const Representation* representation = nullptr;
    const auto compiled =
        Representation::compile(schema, arena)
            .visit(
                [&](const Representation& value) {
                  representation = &value;
                  return Binding::Status::Satisfied;
                },
                [](Ttx::Data::Status) { return Binding::Status::Rejected; });
    if (compiled != Binding::Status::Satisfied) {
      return compiled;
    }

    const auto connected =
        access.connect(Flow::reader(*representation), input.get_data());
    if (connected != Flow::Status::Success) {
      return status(connected);
    }

    // Direct and Shared already supply a safe byte address. Block and Fragment
    // require materialization, which belongs to this publication so dialects
    // can reuse it for token spelling and diagnostics.
    Core::View::Bytes text;
    const auto borrow = [&](const void* data) {
      text = Core::View::Bytes(static_cast<const U8*>(data), size);
      return Ttx::Data::Status::Success;
    };
    const auto copy = [&] {
      auto bytes = arena.allocate(size);
      const Storage target(ttx_storage{representation, bytes.get_data(), size});
      const auto copied = Ttx::Semantic::Flows::Copy::flow(access, target);
      if (copied == Ttx::Data::Status::Success) {
        text = Core::View::Bytes(bytes.get_data(), size);
      }

      return copied;
    };
    const auto observed = access.visit(
        borrow, borrow, [&](auto, auto) { return copy(); },
        [&](auto) { return copy(); });
    if (observed != Ttx::Data::Status::Success) {
      return Binding::Status::Rejected;
    }

    content = Source::Contents::Memory(text, *representation);
    const auto& stream = arena.construct<Source::Lexical::Tokenizer>(
        arena, text, Core::View::Bytes(), source);
    cursor = Source::Lexical::Cursors::Stream(stream, errors);
    return Binding::Status::Satisfied;
  }

  auto supports(System::Uuid id) const -> Binding::Status {
    if (id == Source::Lexical::Cursor::contract_id) {
      return Binding::Status::Satisfied;
    }

    return content->supports(id);
  }

  auto bind_interface(System::Uuid id, Storage output) const
      -> Binding::Status {
    if (id == Source::Lexical::Cursor::contract_id) {
      return Binding::provide<Source::Lexical::Cursor>(
          cursor->get_interface().get_abi(), output);
    }

    return content->bind_interface(id, output);
  }

 private:
  Abstract source;
  Memory::Allocator::Arena arena;
  Flow access;
  Core::Option<Source::Contents::Memory> content;
  Source::Errors errors;
  Core::Option<Source::Lexical::Cursors::Stream> cursor;
};

static auto tokenize(const void*, ttx_abstract source, ttx_publication* output)
    -> ttx_binding_status {
  const Abstract input(source);
  return input.bind<Source::Content>().visit(
      [&](Source::Content content) -> ttx_binding_status {
        auto* observation = new LexicalPublication(input);
        const auto result = observation->read(content);
        if (result != Binding::Status::Satisfied) {
          delete observation;
          return static_cast<ttx_binding_status>(result);
        }

        *output = {
          Abstract::provide(*observation).get_query(), [](const void* self) {
            delete static_cast<const LexicalPublication*>(self);
          }};
        return TTX_BINDING_SATISFIED;
      },
      [](Binding::Failure failure) {
        return static_cast<ttx_binding_status>(failure);
      });
}

auto Source::Lexical::Tokenization::supports(System::Uuid id) const
    -> Binding::Status {
  return id == Source::Tokenization::contract_id ? Binding::Status::Satisfied
                                                 : Binding::Status::Unsupported;
}

auto Source::Lexical::Tokenization::bind_interface(
    System::Uuid id,
    Storage output) const -> Binding::Status {
  if (id != Source::Tokenization::contract_id) {
    return Binding::Status::Unsupported;
  }

  const Source::Tokenization::Api api = {this, tokenize};
  return Binding::provide<Source::Tokenization>(api, output);
}
