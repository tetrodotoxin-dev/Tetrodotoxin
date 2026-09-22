// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Call represents one complete postfix invocation and keeps its argument Pack.
// The receiver's completed result chooses Static or Self lookup. Composite has
// already admitted one Callable for that name and role, which lets the Call
// validate the selected signature without rebuilding an overload search.
class Call : public Expression {
 public:
  TTX_CONTRACT(Call, Expression);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Tetrodotoxin::Source::Lexical::Token name_token,
      Perimortem::Core::View::Bytes name,
      Language::Model::Pack& arguments,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Call&;

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Pack& receiver,
      Perimortem::Core::View::Bytes name,
      Language::Model::Pack& arguments) -> Call&;

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope = {})
      -> Bool override;

  auto link_restored(
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME(name);

  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override;
  auto get_result() const -> const Tetrodotoxin::Source::Abstract& override;
  auto get_type() const -> const Tetrodotoxin::Source::Abstract& override;
  auto get_value_type(Count index) const
      -> const Tetrodotoxin::Source::Abstract& override;
  auto get_layout() const -> const Tetrodotoxin::Source::Layout& override;
  auto resolve() const -> const Tetrodotoxin::Source::Abstract& override;
  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void override;

  auto get_callable() const -> Perimortem::Core::Option<const Model::Callable&>;

  // Fitting already records which parameter accepted each argument output.
  // This query turns that evidence into one label at an authored insertion
  // point. Named arguments carry their own labels, and a composed Pack has one
  // useful insertion point, so its first output represents the whole Pack.
  auto get_argument_parameter(Count index) const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Addressable&>;

  constexpr auto get_arguments() const -> const Language::Model::Pack& {
    return arguments;
  }

  constexpr auto get_receiver() const -> const Model::Pack& { return receiver; }

  constexpr auto get_name_token() const -> Tetrodotoxin::Source::Lexical::Token {
    return name_token;
  }

 public:
  // Each fitted input keeps the parameter and the exact slice of the source
  // Pack Layout that reached it. Lowering can reuse that decision without a
  // parallel producer map.
  class Input {
   public:
    constexpr Input(
        const Tetrodotoxin::Source::Addressable& parameter,
        const Language::Model::Pack& source,
        Count offset,
        Count size)
        : parameter(parameter), source(source), offset(offset), size(size) {}

    constexpr auto get_parameter() const -> const Tetrodotoxin::Source::Addressable& {
      return parameter.get();
    }

    constexpr auto get_source() const -> const Language::Model::Pack& {
      return source.get();
    }

    constexpr auto get_offset() const -> Count { return offset; }

    constexpr auto get_size() const -> Count { return size; }

   private:
    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Addressable> parameter;
    Tetrodotoxin::Source::PackReference<const Language::Model::Pack> source;
    Count offset;
    Count size;
  };

  constexpr auto get_fitted_inputs() const { return fitted_inputs.get_view(); }

 private:
  constexpr Call(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Tetrodotoxin::Source::Lexical::Token name_token,
      Perimortem::Core::View::Bytes name,
      Language::Model::Pack& arguments,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
      : Expression(anchor),
        domain(domain),
        receiver(receiver),
        name_token(name_token),
        name(name),
        arguments(arguments),
        fitted_inputs(domain) {}

  auto fit_inputs(
      const Model::Callable& selected,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Layout&> inputs) -> Bool;

  auto evaluate() -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Model::Pack&>, Error> override;

  Perimortem::Memory::Allocator::Arena& domain;
  Model::Pack& receiver;
  Tetrodotoxin::Source::Lexical::Token name_token;
  Perimortem::Core::View::Bytes name;
  Language::Model::Pack& arguments;
  Perimortem::Core::Option<Tetrodotoxin::Source::Reference<const Model::Callable>>
      callable;
  Perimortem::Core::Option<const Tetrodotoxin::Source::Layout&> input_layout;
  Perimortem::Core::Option<const Tetrodotoxin::Source::Layout&> output;
  Perimortem::Memory::Managed::Vector<Input> fitted_inputs;
};

}  // namespace Tetrodotoxin::Library::Language::Access
