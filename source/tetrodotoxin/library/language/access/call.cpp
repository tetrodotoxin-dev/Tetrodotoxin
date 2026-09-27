// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/access/call.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/diagnostics.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;

static auto select_type(const Abstract& candidate)
    -> Core::Option<const Language::Model::Type&> {
  auto direct = candidate.select<Language::Model::Type>();
  if (direct) {
    return *direct;
  }

  return candidate.resolve().select<Language::Model::Type>();
}

auto Language::Access::Call::create_synthetic(
    Memory::Allocator::Arena& arena,
    Model::Pack& receiver,
    Core::View::Bytes name,
    Language::Model::Pack& arguments) -> Call& {
  return Expression::create_synthetic<Call>(arena, [&](auto source) -> Call {
    return Call(arena, receiver, {}, name, arguments, source);
  });
}

auto Language::Access::Call::create_authored(
    Memory::Allocator::Arena& domain,
    Model::Pack& receiver,
    Token name_token,
    Core::View::Bytes name,
    Language::Model::Pack& arguments,
    Anchor anchor) -> Call& {
  return Expression::create_authored<Call>(
      domain, anchor, [&](Core::Option<Anchor> source) -> Call {
        return Call(domain, receiver, name_token, name, arguments, source);
      });
}

static auto select_result_type(const Abstract& result)
    -> Core::Option<const Language::Model::Type&> {
  auto addressable = result.select<Tetrodotoxin::Source::Addressable>();
  if (addressable) {
    return select_type(addressable->get_type());
  }

  auto direct = result.select<Language::Model::Type>();
  if (direct) {
    return *direct;
  }

  const Abstract& resolved = result.resolve();
  addressable = resolved.select<Tetrodotoxin::Source::Addressable>();
  return addressable ? select_type(addressable->get_type())
                     : select_type(resolved);
}

// The Callable describes the result shape shared by every invocation. This
// Layout borrows that shape while keeping the Call as its producer, so fitted
// flow still points back to the invocation that created it. Borrowing also
// keeps result entries, names, and Types in one place.
static auto create_layout(
    Memory::Allocator::Arena& domain,
    const Language::Access::Call& call,
    const Language::Model::Callable& callable) -> const Tetrodotoxin::Source::Layout& {
  class Layout final : public Tetrodotoxin::Source::Layout {
   public:
    constexpr Layout(
        const Language::Access::Call& call,
        const Language::Model::Callable& callable)
        : call(call), callable(callable) {}

    constexpr auto get_size() const -> Count override {
      return callable.get_results().get_size();
    }

    constexpr auto get_abstract(Count index) const
        -> Core::Option<const Abstract&> override {
      BAIL_IF(index >= get_size());
      return call;
    }

    constexpr auto get_name(Count index) const
        -> Core::Option<Core::View::Bytes> override {
      return callable.get_results().get_name(index);
    }

    auto fits_entry(
        const Tetrodotoxin::Source::Layout& target,
        Count source_index,
        Count target_index) const -> Bool override {
      return callable.get_results().fits_entry(
          target, source_index, target_index);
    }

    auto fits_at(const Tetrodotoxin::Source::Layout& target, Count target_offset) const
        -> Bool override {
      return callable.get_results().fits_at(target, target_offset);
    }

    auto get_fitted_at(
        const Tetrodotoxin::Source::Layout& target,
        Count target_offset,
        Count target_index) const
        -> Utility::Result<const Abstract&, Errors> override {
      if (target_index >= get_size()) {
        return Errors::IndexOutOfBounds;
      }

      if (!has_target_segment(target, target_offset)) {
        return Errors::SizeMismatch;
      }

      if (!fits_at(target, target_offset)) {
        return Errors::IncompatibleFit;
      }
      return call;
    }

   private:
    const Language::Access::Call& call;
    const Language::Model::Callable& callable;
  };

  return domain.construct<Layout>(call, callable);
}

// A Self Call contributes its real receiver as the first runtime input, then
// follows it with the authored argument Pack. Static lookup uses a Type only as
// context, so its runtime input is already the argument Pack itself.
static auto create_inputs(
    Memory::Allocator::Arena& domain,
    const Language::Model::Pack& receiver,
    const Language::Model::Pack& arguments) -> const Tetrodotoxin::Source::Layout& {
  class Inputs final : public Tetrodotoxin::Source::Layout {
   public:
    constexpr Inputs(
        const Language::Model::Pack& receiver,
        const Language::Model::Pack& arguments)
        : receiver(receiver), arguments(arguments) {}

    auto get_size() const -> Count override {
      return 1 + arguments.get_layout().get_size();
    }

    auto get_abstract(Count index) const
        -> Core::Option<const Abstract&> override {
      if (index == 0) {
        return receiver.get_layout().get_abstract(0);
      }
      return arguments.get_layout().get_abstract(index - 1);
    }

    auto get_name(Count index) const
        -> Core::Option<Core::View::Bytes> override {
      return index == 0 ? Core::Option<Core::View::Bytes>()
                        : arguments.get_layout().get_name(index - 1);
    }

    auto fits_entry(
        const Tetrodotoxin::Source::Layout& target,
        Count source,
        Count target_index) const -> Bool override {
      BAIL_IF(source >= get_size() || target_index >= target.get_size());
      if (source != 0) {
        return arguments.fits_entry(target, source - 1, target_index);
      }

      auto target_entry = target.get_abstract(target_index);
      BAIL_IF(!target_entry);
      return select_result_type(*target_entry)
          .visit(
              []() { return False; },
              [&](const Language::Model::Type& type) {
                return receiver.fits(type);
              });
    }

    auto fits_at(const Tetrodotoxin::Source::Layout& target, Count target_offset) const
        -> Bool override {
      BAIL_IF(
          target_offset > target.get_size() ||
          get_size() > target.get_size() - target_offset);
      return fits_entry(target, 0, target_offset) &&
             arguments.fits_at(target, target_offset + 1);
    }

    auto get_fitted_at(
        const Tetrodotoxin::Source::Layout& target,
        Count target_offset,
        Count target_index) const
        -> Utility::Result<const Abstract&, Errors> override {
      if (target_index >= get_size()) {
        return Errors::IndexOutOfBounds;
      }

      if (target_offset > target.get_size() ||
          get_size() > target.get_size() - target_offset) {
        return Errors::SizeMismatch;
      }

      if (!fits_at(target, target_offset)) {
        return Errors::IncompatibleFit;
      }
      if (target_index == 0) {
        auto identity = receiver.get_layout().get_abstract(0);
        return identity ? Utility::Result<const Abstract&, Errors>(*identity)
                        : Utility::Result<const Abstract&, Errors>(
                              Errors::IncompatibleFit);
      }
      return arguments.get_fitted_at(
          target, target_offset + 1, target_index - 1);
    }

   private:
    const Language::Model::Pack& receiver;
    const Language::Model::Pack& arguments;
  };

  return domain.construct<Inputs>(receiver, arguments);
}

auto Language::Access::Call::link(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  if (!get_anchor() && callable && output) {
    return True;
  }

  // Completing the receiver tells invocation lookup which role the source
  // actually produced. A Type leads to Static lookup, while an Addressable
  // leads to Self lookup with a runtime receiver.
  BAIL_IF(!receiver.link(cursor, lexical_context, access_scope));
  BAIL_IF(!arguments.link(cursor, lexical_context, access_scope));
  // Static lookup uses the Type as context, while argument positions still
  // describe value flow. Keeping that split here gives fitting the exact Pack
  // Layout it expects.
  if (!arguments.is_complete()) {
    cursor.create_expression_error(
        get_anchor(),
        "Library invocation arguments did not produce value flow."_view,
        "Use Type results only as access receivers."_view);
    return False;
  }

  const Abstract& receiver_result = receiver.get_result();
  const Abstract& host = access_scope.visit(
      [&]() -> const Abstract& { return lexical_context; },
      [](const Abstract& selected) -> const Abstract& { return selected; });
  const Abstract& candidate = receiver_result.visit<Language::Model::Type>(
      [&](const Language::Model::Type& type) -> const Abstract& {
        return type.resolve_concept("static"_view).resolve_concept(name);
      },
      [&](const Abstract& receiver) -> const Abstract& {
        auto addressable = receiver.resolve().select<Tetrodotoxin::Source::Addressable>();
        if (addressable) {
          return addressable->get_type()
              .resolve()
              .resolve_concept("instance"_view)
              .resolve_concept(name);
        }
        auto value_type =
            this->receiver.get_type().resolve().select<Language::Model::Type>();
        return value_type ? value_type->resolve_concept("instance"_view)
                                .resolve_concept(name)
                          : receiver.resolve_concept("static"_view)
                                .resolve_concept(name);
      });
  auto selected = candidate.resolve().select<Language::Model::Callable>();
  if (!selected) {
    auto report = cursor.create_report(get_anchor());
    report << "Receiver '"_view << receiver_result.get_name()
           << "' has no accessible Callable named '"_view << name << "'. "_view
           << "Receiver type: "_view;
    Language::Diagnostics::write_type(report, receiver_result);
    report << "."_view;
    report.get_hint()
        << "Correct the Callable spelling or invoke it through the required "
           "Static or Self receiver."_view;
    return False;
  }

  auto function = selected->select<Language::Function>();
  if (function && function->get_definition().get_visibility() ==
                      Tetrodotoxin::Language::Visibility::Private) {
    auto caller_type = host.select<Language::Model::Type>();
    if (!caller_type ||
        !caller_type->has_private_access_to(function->get_host())) {
      cursor.create_expression_error(
          get_anchor(), "Callable is private to its declaring Type."_view,
          "Invoke it only from code hosted by that Type."_view);
      return False;
    }
  }

  if (!selected->accepts_receiver(receiver_result, host)) {
    auto report = cursor.create_report(get_anchor());
    report
        << "Callable '"_view << selected->get_name()
        << "' cannot use receiver '"_view << receiver_result.get_name()
        << "' because its required storage or authority is unavailable."_view;
    report.get_hint()
        << "Invoke through an Addressable whose lifetime and write authority "
           "satisfy this Callable."_view;
    return False;
  }

  const Tetrodotoxin::Source::Layout& parameters = selected->get_parameters();
  Bool arguments_fit = arguments.fits(parameters);
  if (selected->is_type_bound()) {
    if (!input_layout) {
      input_layout = create_inputs(domain, receiver, arguments);
    }
    arguments_fit = input_layout->fits(parameters);
  }

  if (!arguments_fit) {
    auto report = cursor.create_report(get_anchor());
    report << "Arguments do not fit Callable '"_view << selected->get_name()
           << "'.\nSource produces: "_view;
    if (selected->is_type_bound()) {
      Language::Diagnostics::write_layout(report, *input_layout);
    } else {
      Language::Diagnostics::write_pack(report, arguments);
    }
    report << "\nParameters accept: "_view;
    Language::Diagnostics::write_layout(report, parameters);
    report.get_hint()
        << "Supply the exact positional or named parameter Types shown above."_view;
    return False;
  }

  if (callable && &callable->get() != &*selected) {
    auto report = cursor.create_report(get_anchor());
    report << "Internal semantic error: invocation '"_view << name
           << "' changed Callable identity from '"_view
           << callable->get().get_name() << "' to '"_view
           << selected->get_name() << "'."_view;
    report.get_hint()
        << "The source is valid; report this unstable linking result."_view;
    return False;
  }

  if (callable) {
    // Repeated linking can revalidate the surrounding graph. Reusing the
    // published producer Layout keeps this invocation's value identity stable.
    return True;
  }

  fitted_inputs.clear();
  if (!fit_inputs(*selected, input_layout)) {
    cursor.create_expression_error(
        get_anchor(),
        "Internal semantic error: Callable fitting did not retain one stable "
        "input mapping."_view,
        "The source is valid; report this invocation fitting failure."_view);
    return False;
  }

  const Tetrodotoxin::Source::Layout& retained_output =
      create_layout(domain, *this, *selected);
  callable = Reference<const Language::Model::Callable>(*selected);
  output = retained_output;
  auto source_anchor = get_anchor();
  if (source_anchor) {
    cursor.get_associations().create(*source_anchor, *selected);
  }

  // Invocation completion follows the complete result Layout rather than the
  // scalar Type shortcut in Expression. Empty and multiple results can then
  // complete normally while the selected Callable remains their shape owner.
  return True;
}

auto Language::Access::Call::link_restored(
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  BAIL_IF(
      !receiver.link_restored(lexical_context, access_scope) ||
      !arguments.link_restored(lexical_context, access_scope) ||
      !arguments.is_complete());

  const Abstract& receiver_result = receiver.get_result();
  const Abstract& host = access_scope.visit(
      [&]() -> const Abstract& { return lexical_context; },
      [](const Abstract& selected) -> const Abstract& { return selected; });
  const Abstract& candidate = receiver_result.visit<Language::Model::Type>(
      [&](const Language::Model::Type& type) -> const Abstract& {
        return type.resolve_concept("static"_view).resolve_concept(name);
      },
      [&](const Abstract& selected) -> const Abstract& {
        auto addressable = selected.resolve().select<Tetrodotoxin::Source::Addressable>();
        if (addressable) {
          return addressable->get_type()
              .resolve()
              .resolve_concept("instance"_view)
              .resolve_concept(name);
        }
        auto value_type =
            this->receiver.get_type().resolve().select<Language::Model::Type>();
        return value_type ? value_type->resolve_concept("instance"_view)
                                .resolve_concept(name)
                          : selected.resolve_concept("static"_view)
                                .resolve_concept(name);
      });
  auto selected = candidate.resolve().select<Language::Model::Callable>();
  BAIL_IF(!selected || !selected->accepts_receiver(receiver_result, host));
  auto function = selected->select<Language::Function>();
  if (function && function->get_definition().get_visibility() ==
                      Tetrodotoxin::Language::Visibility::Private) {
    auto caller_type = host.select<Language::Model::Type>();
    BAIL_IF(
        !caller_type ||
        !caller_type->has_private_access_to(function->get_host()));
  }

  const Layout& parameters = selected->get_parameters();
  Bool fits = arguments.fits(parameters);
  if (selected->is_type_bound()) {
    input_layout = create_inputs(domain, receiver, arguments);
    fits = input_layout->fits(parameters);
  }
  BAIL_IF(!fits || !fit_inputs(*selected, input_layout));

  callable = Reference<const Language::Model::Callable>(*selected);
  output = create_layout(domain, *this, *selected);
  return True;
}

auto Language::Access::Call::get_documentation() const -> const Tetrodotoxin::Source::Documentation& {
  return callable.visit(
      []() -> const Tetrodotoxin::Source::Documentation& { return Tetrodotoxin::Source::Documentation::get_empty(); },
      [](const Reference<const Language::Model::Callable>& selected)
          -> const Tetrodotoxin::Source::Documentation& {
        return selected.get().get_documentation();
      });
}

auto Language::Access::Call::get_result() const -> const Abstract& {
  return callable.visit(
      [this]() -> const Abstract& { return *this; },
      [this](const Reference<const Language::Model::Callable>& selected)
          -> const Abstract& {
        auto self = selected.get().get_self_result();
        return self ? static_cast<const Abstract&>(*self)
                    : static_cast<const Abstract&>(*this);
      });
}

auto Language::Access::Call::get_type() const -> const Abstract& {
  if (!callable) {
    return Unknown::get_unknown();
  }
  const Tetrodotoxin::Source::Layout& results = callable->get().get_results();
  if (results.get_size() != 1) {
    return Unknown::get_unknown();
  }

  return results.get_abstract(0).visit(
      []() -> const Abstract& { return Unknown::get_unknown(); },
      [](const Abstract& result) -> const Abstract& {
        return select_result_type(result).visit(
            []() -> const Abstract& { return Unknown::get_unknown(); },
            [](const Language::Model::Type& type) -> const Abstract& {
              return type;
            });
      });
}

auto Language::Access::Call::get_value_type(Count index) const
    -> const Abstract& {
  if (!callable) {
    return Unknown::get_unknown();
  }
  const Tetrodotoxin::Source::Layout& results = callable->get().get_results();
  auto result = results.get_abstract(index);
  if (!result) {
    return Unknown::get_unknown();
  }
  return select_result_type(*result).visit(
      []() -> const Abstract& { return Unknown::get_unknown(); },
      [](const Language::Model::Type& type) -> const Abstract& {
        return type;
      });
}

auto Language::Access::Call::get_layout() const -> const Tetrodotoxin::Source::Layout& {
  return *output;
}

auto Language::Access::Call::resolve() const -> const Abstract& {
  if (!callable || !output) {
    return Unknown::get_unknown();
  }

  return *this;
}

auto Language::Access::Call::finalize(Cursor& cursor) -> void {
  // Receiver and argument Pack are the complete evaluation inputs for this
  // invocation. When a Block retains the Call directly, evaluating those
  // inputs is the effect itself even when the result flow is discarded.
  receiver.finalize(cursor);
  arguments.finalize(cursor);
}

static auto select_parameter(
    const Language::Model::Callable& callable,
    Count index) -> Core::Option<const Tetrodotoxin::Source::Addressable&> {
  auto entry = callable.get_parameters().get_abstract(index);
  return entry ? entry->select<Tetrodotoxin::Source::Addressable>()
               : Core::Option<const Tetrodotoxin::Source::Addressable&>();
}

auto Language::Access::Call::fit_inputs(
    const Language::Model::Callable& callable,
    Core::Option<const Tetrodotoxin::Source::Layout&> input_layout) -> Bool {
  const Tetrodotoxin::Source::Layout& parameters = callable.get_parameters();
  Count receiver_offset = callable.declares_self() ? 1 : 0;
  Count source_size = receiver_offset + arguments.get_layout().get_size();
  if (source_size == parameters.get_size()) {
    const Tetrodotoxin::Source::Layout& source =
        input_layout ? *input_layout : arguments.get_layout();
    for (Count target_index = 0; target_index < parameters.get_size();
         target_index++) {
      Count selected = 0;
      Count matches = 0;
      for (Count source_index = 0; source_index < source_size; source_index++) {
        if (!source.get_name(source_index) && source_index != target_index) {
          continue;
        }

        Bool fits = input_layout ? input_layout->fits_entry(
                                       parameters, source_index, target_index)
                                 : arguments.fits_entry(
                                       parameters, source_index, target_index);
        if (fits) {
          selected = source_index;
          matches++;
        }
      }

      auto parameter = select_parameter(callable, target_index);
      BAIL_IF(matches != 1 || !parameter);
      fitted_inputs.insert(
          selected < receiver_offset
              ? Input(*parameter, receiver, 0, 1)
              : Input(*parameter, arguments, selected - receiver_offset, 1));
    }

    return True;
  }

  BAIL_IF(parameters.get_size() != receiver_offset + 1);
  if (receiver_offset != 0) {
    auto parameter = select_parameter(callable, 0);
    BAIL_IF(!parameter);
    fitted_inputs.insert(Input(*parameter, receiver, 0, 1));
  }

  auto parameter = select_parameter(callable, receiver_offset);
  BAIL_IF(!parameter);
  fitted_inputs.insert(
      Input(*parameter, arguments, 0, arguments.get_layout().get_size()));
  return True;
}

auto Language::Access::Call::evaluate()
    -> Utility::Result<Core::Option<Language::Model::Pack&>, Error> {
  auto selected = get_callable();
  if (!selected) {
    return Core::Option<Language::Model::Pack&>();
  }

  Core::Option<const Language::Model::Pack&> folded_receiver;
  if (!selected->declares_self()) {
    return selected->fold_call(domain, folded_receiver, arguments);
  }

  return Expression::fold(receiver).visit(
      [&](const Core::Option<Language::Model::Pack&>& value)
          -> Utility::Result<Core::Option<Language::Model::Pack&>, Error> {
        if (!value) {
          return Core::Option<Language::Model::Pack&>();
        }

        folded_receiver = *value;
        return selected->fold_call(domain, folded_receiver, arguments);
      },
      [](const Error& error)
          -> Utility::Result<Core::Option<Language::Model::Pack&>, Error> {
        return error;
      });
}

auto Language::Access::Call::get_callable() const
    -> Core::Option<const Language::Model::Callable&> {
  return callable.visit(
      []() -> Core::Option<const Language::Model::Callable&> { return {}; },
      [](const Reference<const Language::Model::Callable>& selected)
          -> Core::Option<const Language::Model::Callable&> {
        return selected.get();
      });
}

auto Language::Access::Call::get_argument_parameter(Count index) const
    -> Core::Option<const Tetrodotoxin::Source::Addressable&> {
  const Tetrodotoxin::Source::Layout& layout = arguments.get_layout();
  BAIL_IF(index >= layout.get_size() || layout.get_name(index));
  for (const Input& input : fitted_inputs.get_view()) {
    if (&input.get_source() != &arguments || index < input.get_offset() ||
        index >= input.get_offset() + input.get_size()) {
      continue;
    }
    return index == input.get_offset()
               ? Core::Option<const Tetrodotoxin::Source::Addressable&>(
                     input.get_parameter())
               : Core::Option<const Tetrodotoxin::Source::Addressable&>();
  }
  return {};
}
