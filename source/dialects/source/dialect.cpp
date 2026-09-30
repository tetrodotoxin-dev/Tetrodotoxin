// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/dialects/source/dialect.hpp"

#include "perimortem/core/object.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/dialects/source/input.hpp"
#include "tetrodotoxin/dialects/source/tokenizer.hpp"
#include "ttx/concept/capabilities/borrow.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Dialects;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;

// One native owner retains the bytes and lexical observations together. The
// Object carrier lets Borrow reserve that same result without a second token
// inventory. Separate observation and acquired tables preserve the release
// policy when a consumer binds another interface on its acquired answer.
class SourceText {
 public:
  SourceText(
      Object<> storage,
      tetrodotoxin_source_input input,
      Option<Policies::Borrowed> retained,
      View::Bytes bytes)
      : storage(storage),
        path(View::Bytes(input.path.data, input.path.size)),
        retained(retained),
        copied(retained ? View::Bytes() : bytes),
        text(retained ? bytes : copied.get_view()),
        tokenizer(arena, text, path.get_view()) {}

  ~SourceText() {
    if (retained) {
      retained->release();
    }
  }

  auto is_valid() const -> Bool { return tokenizer.is_valid(); }

  static auto create(tetrodotoxin_source_input input)
      -> Perimortem::Utility::Result<SourceText&, Binding::Failure> {
    static constexpr Object<>::Descriptor descriptor{
      sizeof(SourceText), alignof(SourceText), [](U8* payload) {
        reinterpret_cast<SourceText*>(payload)->~SourceText();
      }};
    Option<Policies::Borrowed> retained;
    const Abstract subject(input.text);
    subject.bind<Capabilities::Borrow>().visit(
        [&](Capabilities::Borrow borrow) {
          borrow.borrow().visit(
              [&](Policies::Borrowed answer) { retained = answer; },
              [](Binding::Failure) {});
        },
        [](Binding::Failure) {});
    // Borrow may supply a different object. Observe that acquired answer only
    // after acquisition so its retained storage supplies the exact token bytes.
    const auto bytes = retained ? retained->get_data() : subject.get_data();
    if ((!bytes.get_data() && bytes.get_size()) || bytes.get_size() > 65535) {
      if (retained) {
        retained->release();
      }
      return Binding::Failure::Rejected;
    }
    const auto storage = Object<>::create(descriptor);
    return *new (storage.get_payload(), Placement::Construct)
        SourceText(storage, input, retained, bytes);
  }

  auto release() const -> void { storage.release(); }

  auto borrowed() const -> Policies::Borrowed {
    static const ttx_borrowed_ops operations{
      stream<true>().get_abi().operations->abstract, [](const void* source) {
        static_cast<const SourceText*>(source)->release();
      }};
    return Policies::Borrowed({this, &operations});
  }

  template <bool Acquired>
  auto stream() const -> Source::Stream {
    static const tetrodotoxin_source_stream_ops operations{
      {
        [](const void*, perimortem_uuid id) -> ttx_binding_status {
          const Perimortem::System::Uuid contract(id);
          if (contract == Source::Stream::contract_id ||
              contract == Abstract::contract_id ||
              contract == Capabilities::Borrow::contract_id) {
            return TTX_BINDING_SATISFIED;
          }
          if (contract == Policies::Borrowed::contract_id) {
            return Acquired ? TTX_BINDING_SATISFIED : TTX_BINDING_REJECTED;
          }
          return TTX_BINDING_UNKNOWN;
        },
        [](const void* source, perimortem_uuid id,
           ttx_storage requested) -> ttx_binding_status {
          const Perimortem::System::Uuid contract(id);
          const Ttx::Data::Form::Storage target(requested);
          const auto& owner = *static_cast<const SourceText*>(source);
          if (contract == Source::Stream::contract_id) {
            return static_cast<ttx_binding_status>(
                Binding::provide<Source::Stream>(
                    {source, &operations}, target));
          }
          if (contract == Abstract::contract_id) {
            return static_cast<ttx_binding_status>(Binding::provide<Abstract>(
                {source, &operations.abstract}, target));
          }
          if (contract == Capabilities::Borrow::contract_id) {
            static const ttx_borrow_ops borrow_operations{
              operations.abstract,
              [](const void* source,
                 ttx_borrowed* output) -> ttx_binding_status {
                const auto& owner = *static_cast<const SourceText*>(source);
                owner.storage.retain();
                *output = owner.borrowed().get_abi();
                return TTX_BINDING_SATISFIED;
              }};
            return static_cast<ttx_binding_status>(
                Binding::provide<Capabilities::Borrow>(
                    {source, &borrow_operations}, target));
          }
          if (contract == Policies::Borrowed::contract_id) {
            if constexpr (Acquired) {
              return static_cast<ttx_binding_status>(
                  Binding::provide<Policies::Borrowed>(
                      owner.borrowed().get_abi(), target));
            }
            return TTX_BINDING_REJECTED;
          }
          return TTX_BINDING_UNKNOWN;
        },
        [](const void* source) -> perimortem_view_bytes {
          const auto& text = static_cast<const SourceText*>(source)->text;
          return {text.get_data(), text.get_size()};
        },
        [](const void* source) -> ttx_abstract {
          return {source, &operations.abstract};
        },
        [](const void*, perimortem_view_bytes) { return ttx_unknown(); },
        [](const void*, ttx_concept_visitor) {},
      },
      [](const void* source) -> tetrodotoxin_source_snapshot {
        const auto& owner = *static_cast<const SourceText*>(source);
        return {
          {owner.path.get_view().get_data(), owner.path.get_size()},
          {owner.text.get_data(), owner.text.get_size()}};
      },
      [](const void* source) -> tetrodotoxin_source_tokens {
        return static_cast<const SourceText*>(source)
            ->tokenizer.get_token_buffer();
      },
      [](const void*) -> perimortem_uuid {
        return Source::Stream::lexical_contract;
      }};
    return Source::Stream({this, &operations});
  }

 private:
  Object<> storage;
  Dynamic::Bytes path;
  Option<Policies::Borrowed> retained;
  Dynamic::Bytes copied;
  View::Bytes text;
  Allocator::Arena arena;
  Source::Tokenizer tokenizer;
};

auto Source::Dialect::importer() -> Capabilities::Import {
  static constexpr U8 identity = 0;
  static const ttx_import_ops operations{
    {
      [](const void*, perimortem_uuid id) -> ttx_binding_status {
        const Perimortem::System::Uuid contract(id);
        return contract == Capabilities::Import::contract_id ||
                       contract == Abstract::contract_id
                   ? TTX_BINDING_SATISFIED
                   : TTX_BINDING_UNKNOWN;
      },
      [](const void* source, perimortem_uuid id,
         ttx_storage requested) -> ttx_binding_status {
        const Perimortem::System::Uuid contract(id);
        const Ttx::Data::Form::Storage target(requested);
        if (contract == Capabilities::Import::contract_id) {
          return static_cast<ttx_binding_status>(
              Binding::provide<Capabilities::Import>(
                  {source, &operations}, target));
        }
        if (contract == Abstract::contract_id) {
          return static_cast<ttx_binding_status>(Binding::provide<Abstract>(
              {source, &operations.abstract}, target));
        }
        return TTX_BINDING_UNKNOWN;
      },
      [](const void*) -> perimortem_view_bytes {
        return {reinterpret_cast<const U8*>("Source"), 6};
      },
      [](const void* source) -> ttx_abstract {
        return {source, &operations.abstract};
      },
      [](const void*, perimortem_view_bytes) { return ttx_unknown(); },
      [](const void*, ttx_concept_visitor) {},
    },
    [](const void*, const void* input, const ttx_representation* representation,
       void* receiver,
       void (*receive)(void*, ttx_abstract)) -> ttx_binding_status {
      const auto& expected = *tetrodotoxin_source_input_representation();
      if (!input || !representation || !receive ||
          !expected.compatible(*representation)) {
        return TTX_BINDING_REJECTED;
      }
      const auto& value = *static_cast<const tetrodotoxin_source_input*>(input);
      if ((!value.path.data && value.path.size) ||
          !Abstract::accept(value.text)) {
        return TTX_BINDING_REJECTED;
      }

      // The invocation owns one reservation until the callback returns.
      // Borrow can add independent reservations while that observation lives.
      return SourceText::create(value).visit(
          [&](SourceText& owner) -> ttx_binding_status {
            if (!owner.is_valid()) {
              owner.release();
              return TTX_BINDING_REJECTED;
            }
            receive(receiver, owner.stream<false>().Abstract::get_abi());
            owner.release();
            return TTX_BINDING_SATISFIED;
          },
          [](Binding::Failure failure) {
            return static_cast<ttx_binding_status>(failure);
          });
    }};
  return Capabilities::Import({&identity, &operations});
}
