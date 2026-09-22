// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/generic.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/language/import.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

static auto matches_parameter(
    Language::Generic::Parameters parameter,
    const Language::Generic::Argument& argument) -> Bool {
  switch (parameter) {
  case Language::Generic::Parameters::Type:
    return argument.is<const Language::Model::Type&>();
  case Language::Generic::Parameters::SemanticType:
    return argument.is<Language::Generic::SemanticType>();
  case Language::Generic::Parameters::U64:
    return argument.is<::U64>();
  case Language::Generic::Parameters::S64:
    return argument.is<::S64>();
  case Language::Generic::Parameters::Bool:
    return argument.is<::Bool>();
  }

  return False;
}

static auto matches_key(
    Core::View::Vector<Language::Generic::Argument> retained,
    Core::View::Vector<Language::Generic::Argument> candidate) -> Bool {
  if (retained.get_size() != candidate.get_size()) {
    return False;
  }

  for (Count i = 0; i < candidate.get_size(); i++) {
    if (retained.get_data()[i] != candidate.get_data()[i]) {
      return False;
    }
  }

  return True;
}

Language::Generic::Entry::Entry(
    Memory::Allocator::Arena& domain,
    Core::View::Vector<Argument> source_arguments,
    const Language::Model::Type& value)
    : arguments(domain), value(value) {
  for (Count i = 0; i < source_arguments.get_size(); i++) {
    arguments.insert(source_arguments.get_data()[i]);
  }
}

auto Language::Generic::normalize_argument(
    Parameters parameter,
    const Tetrodotoxin::Source::Abstract& argument) const -> Core::Option<Argument> {
  switch (parameter) {
  case Parameters::Type: {
    const Tetrodotoxin::Source::Abstract& represented =
        argument.is<Tetrodotoxin::Source::Type>() ? argument : argument.resolve();
    const Tetrodotoxin::Source::Abstract& selected =
        represented.is<Tetrodotoxin::Language::Import>()
            ? represented.get_type()
            : represented;
    auto type = selected.select<Language::Model::Type>();
    BAIL_IF(!type);
    return Argument(*type);
  }
  case Parameters::SemanticType: {
    const Tetrodotoxin::Source::Abstract& represented =
        argument.is<Tetrodotoxin::Source::Type>() ? argument : argument.resolve();
    const Tetrodotoxin::Source::Abstract& selected =
        represented.is<Tetrodotoxin::Language::Import>()
            ? represented.get_type()
            : represented;
    auto type = selected.select<Tetrodotoxin::Source::Type>();
    BAIL_IF(!type);
    return Argument(SemanticType::create(*type));
  }
  case Parameters::U64: {
    auto constant = argument.select<Constants::Unsigned>();
    const auto expected =
        context.resolve_concept("U64"_view).select<Language::Model::Type>();
    BAIL_IF(!constant || !expected || &constant->get_type() != &*expected);
    return Argument(constant->get_value());
  }
  case Parameters::S64: {
    auto constant = argument.select<Constants::Signed>();
    const auto expected =
        context.resolve_concept("S64"_view).select<Language::Model::Type>();
    BAIL_IF(!constant || !expected || &constant->get_type() != &*expected);
    return Argument(constant->get_value());
  }
  case Parameters::Bool: {
    auto value = Language::Model::Pack::from(argument);
    BAIL_IF(!value);
    auto actual = value->get_value_type(0)
                      .resolve()
                      .select<Language::Model::Types::Flag>();
    auto expected = context.resolve_concept("Bool"_view)
                        .resolve()
                        .select<Language::Model::Types::Flag>();
    BAIL_IF(!actual || !expected || &actual->resolve() != &expected->resolve());
    auto validity = actual->get_validity(*value);
    BAIL_IF(!validity);
    return Argument(*validity);
  }
  }

  return {};
}

auto Language::Generic::materialize(const Tetrodotoxin::Source::Layout& layout) const
    -> Materialization {
  auto parameters = get_parameterization();
  if (parameters.get_size() != layout.get_size()) {
    return Failure(Failure::Type::Arity);
  }

  // Layout entries are graph edges, but the cache key must contain only the
  // parameter values this Generic owns. Normalization keeps Alias traversal
  // and literal storage details out of canonical Type identity.
  Memory::Dynamic::Vector<Argument> arguments(parameters.get_size());
  for (Count i = 0; i < parameters.get_size(); i++) {
    auto semantic = layout.get_abstract(i);
    if (!semantic) {
      return Failure(Failure::Type::Parameter, i);
    }
    auto argument = normalize_argument(parameters.get_data()[i], *semantic);
    if (!argument) {
      return Failure(Failure::Type::Parameter, i);
    }
    arguments.insert(*argument);
  }

  return materialize(arguments.get_view());
}

auto Language::Generic::validate_materializations(
    Tetrodotoxin::Source::Lexical::Cursor& cursor) const -> Bool {
  Bool valid = True;
  for (Entry* entry : entries.get_view()) {
    valid &= entry->value.validate_layout(cursor);
  }

  return valid;
}

auto Language::Generic::materialize(
    Core::View::Vector<Argument> arguments) const -> Materialization {
  auto parameters = get_parameterization();
  if (&resolve() != this) {
    return Failure(Failure::Type::Unavailable);
  }
  if (parameters.get_size() != arguments.get_size()) {
    return Failure(Failure::Type::Arity);
  }

  for (Count i = 0; i < arguments.get_size(); i++) {
    if (!matches_parameter(parameters.get_data()[i], arguments.get_data()[i])) {
      return Failure(Failure::Type::Parameter, i);
    }

    const Language::Model::Type* type =
        arguments.get_data()[i].find<const Language::Model::Type&>();
    if (type != nullptr) {
      const Tetrodotoxin::Source::Abstract& resolved = type->resolve();
      if (!resolved.is<Tetrodotoxin::Source::Unknown>() && &resolved != type) {
        return Failure(Failure::Type::Parameter, i);
      }
    }
    const SemanticType* semantic_type =
        arguments.get_data()[i].find<SemanticType>();
    if (semantic_type != nullptr) {
      const Tetrodotoxin::Source::Abstract& resolved = semantic_type->get().resolve();
      if (!resolved.is<Tetrodotoxin::Source::Unknown>() &&
          &resolved != &semantic_type->get()) {
        return Failure(Failure::Type::Parameter, i);
      }
    }
  }

  // Completed keys return the one canonical Type. Formula evaluation happens
  // only after this lookup so repeated applications never create shadow Types.
  for (Count i = 0; i < entries.get_size(); i++) {
    Entry& entry = *entries[i];
    if (matches_key(entry.arguments.get_view(), arguments)) {
      return entry.value;
    }
  }

  // The active chain is transaction state rather than another semantic graph.
  // Marking every participant rejects an indirect cycle instead of caching the
  // outer formulas after an inner formula reenters the same key.
  for (Active* candidate = active; candidate != nullptr;
       candidate = candidate->previous) {
    if (matches_key(candidate->arguments, arguments)) {
      for (Active* participant = active; participant != nullptr;
           participant = participant->previous) {
        participant->reentered = True;
      }
      return Failure(Failure::Type::Recursive);
    }
  }

  Active transaction(arguments, active);
  active = &transaction;
  auto created = create(arguments);
  active = transaction.previous;
  if (transaction.reentered) {
    return Failure(Failure::Type::Recursive);
  }
  if (!created || &created->resolve() != &*created) {
    return Failure(Failure::Type::Formula);
  }

  // A formula result enters the cache only after it proves one complete Type.
  // Failed applications therefore leave no identity for a later query to find.
  Entry& entry = domain.construct<Entry>(domain, arguments, *created);
  entries.insert(&entry);
  return entry.value;
}

auto Language::Generic::resolve_concept(Core::View::Bytes) const
    -> const Tetrodotoxin::Source::Abstract& {
  // Applying a Generic is explicit TypeReference syntax. Lending the creating
  // context here would make a selected Generic silently expose unrelated names.
  return Tetrodotoxin::Source::Unknown::get_unknown();
}
