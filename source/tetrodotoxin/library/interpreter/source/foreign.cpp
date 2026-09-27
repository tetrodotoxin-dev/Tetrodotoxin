// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/source/foreign.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/interpreter/declarations/signature.hpp"
#include "tetrodotoxin/library/interpreter/type_reference.hpp"
#include "tetrodotoxin/library/language/foreign.hpp"
#include "tetrodotoxin/source/lexical/lexicon.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

using Tetrodotoxin::Language::Visibility;

template <typename declaration_type>
static auto has_same_declaration(
    const declaration_type& left,
    const declaration_type& right,
    Core::View::Bytes source) -> Bool {
  return left.get_anchor().get_span().caculate_text(source) ==
         right.get_anchor().get_span().caculate_text(source);
}

static auto parse_visibility(
    Cursor& cursor,
    Token& token,
    Visibility& visibility) -> Bool {
  token = cursor.current();
  switch (token.get_code().get_type()) {
  case Code::Type::Public:
    cursor.consume();
    visibility = Visibility::Public;
    return True;
  case Code::Type::Private:
    cursor.consume();
    visibility = Visibility::Private;
    return True;
  case Code::Type::Expose:
    cursor.consume();
    visibility = Visibility::Exposed;
    return True;
  default:
    cursor.create_token_error(
        "Foreign State requires `public` or `expose` visibility."_view);
    return False;
  }
}

static auto parse_state(
    Language::Foreign& host,
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation,
    Core::View::Bytes abi) -> Core::Option<Language::Foreign::State&> {
  Token opening = cursor.current();
  Token visibility_token;
  Visibility visibility = Visibility::Private;
  BAIL_IF(!parse_visibility(cursor, visibility_token, visibility));
  if (visibility == Visibility::Private) {
    cursor.create_token_error(
        visibility_token,
        "Private Foreign State is unreachable from its parent Library."_view,
        "Use `public state` for reads and writes or `expose state` for reads."_view);
    return {};
  }

  Token policy = cursor.current();
  if (policy.get_code() == Code::Type::Const) {
    cursor.create_token_error(
        policy,
        "Foreign const declarations require a loader or embedding contract."_view,
        "Use State for external storage until a compile time value owner is "
        "available."_view);
    return {};
  }
  Token qualifier = cursor.require(
      Code::Type::State,
      "Foreign data declarations require the `state` policy."_view);
  BAIL_IF(!qualifier);
  Token name_token = cursor.require(
      Code::Type::Addressable,
      "Foreign State requires one addressable symbol name."_view);
  BAIL_IF(!name_token);
  BAIL_IF(!cursor.require(
      Code::Type::Define, "Foreign State requires `:` before its Type."_view));
  auto type_reference = Interpreter::TypeReference::parse(host, cursor);
  BAIL_IF(!type_reference);
  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Foreign State requires one terminating `;`."_view);
  BAIL_IF(!terminator);

  auto& definition = Tetrodotoxin::Language::Definition::create_authored(
      cursor, documentation, host, {}, {}, visibility, visibility_token,
      name_token.caculate_text(cursor.get_source_text()), name_token, qualifier,
      Anchor::create(name_token, Span(opening, terminator)));
  return Language::Foreign::State::create_authored(
      cursor.get_arena(), definition, *type_reference, abi);
}

static auto parse_function(
    Language::Foreign& host,
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation,
    Core::View::Bytes abi) -> Core::Option<Language::Foreign::Function&> {
  Token opening = cursor.current();
  Token visibility = cursor.current();
  switch (visibility.get_code().get_type()) {
  case Code::Type::Private:
    cursor.create_token_error(
        visibility,
        "Private Foreign Functions are unreachable from their parent Library."_view,
        "Declare the bodyless external Callable as `public`."_view);
    return {};
  case Code::Type::Expose:
    cursor.create_token_error(
        visibility, "Foreign Functions do not accept `expose` visibility."_view,
        "Declare the bodyless external Callable as `public`."_view);
    return {};
  case Code::Type::Public:
    cursor.consume();
    break;
  default:
    cursor.create_token_error(
        "Foreign Functions require `public` visibility."_view);
    return {};
  }

  Token qualifier = cursor.require(
      Code::Type::Func,
      "Foreign Function declarations require the `func` keyword."_view);
  BAIL_IF(!qualifier);
  Token name_token = cursor.require(
      Code::Type::Addressable,
      "Foreign Function requires one addressable symbol name."_view);
  BAIL_IF(!name_token);
  auto signature = Interpreter::Declarations::Signature::parse(cursor, host);
  BAIL_IF(!signature);
  if (signature->declares_self()) {
    cursor.create_expression_error(
        Anchor::create(Span(name_token, cursor.peek(-1))),
        "Foreign Function cannot declare a `self` receiver."_view,
        "External Callables are selected only through the source Foreign "
        "context."_view);
    return {};
  }
  if (cursor.matches(Code::Type::ScopeStart)) {
    cursor.create_token_error(
        cursor.current(),
        "Foreign Function declarations cannot contain an authored body."_view,
        "Terminate the external signature with `;`."_view);
    return {};
  }

  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Foreign Function requires one terminating `;`."_view);
  BAIL_IF(!terminator);
  auto& definition = Tetrodotoxin::Language::Definition::create_authored(
      cursor, documentation, host, {}, {}, Visibility::Public, visibility,
      name_token.caculate_text(cursor.get_source_text()), name_token, qualifier,
      Anchor::create(name_token, Span(opening, terminator)));
  return Language::Foreign::Function::create_authored(
      cursor.get_arena(), definition, *signature, abi);
}

auto Interpreter::Source::Foreign::is_next(const Cursor& cursor) -> Bool {
  return cursor.matches(Code::Type::Addressable) &&
         cursor.current().caculate_text(cursor.get_source_text()) ==
             "foreign"_view;
}

auto Interpreter::Source::Foreign::parse(
    Language::Foreign& host,
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& block_documentation) -> Bool {
  if (host.get_stage() != Language::Foreign::Stage::Authored ||
      !is_next(cursor)) {
    cursor.create_token_error(
        "Library Foreign blocks require `foreign` before graph completion."_view);
    return False;
  }
  cursor.consume();

  Token abi_token = cursor.require(
      Code::Type::String,
      "Foreign blocks require one quoted ABI selector."_view);
  BAIL_IF(!abi_token);
  Core::View::Bytes quoted = abi_token.caculate_text(cursor.get_source_text());
  if (!Lexicon::validate(Code::Type::String, quoted) || quoted.get_size() < 2) {
    cursor.create_token_error(
        abi_token, "Foreign ABI selector is not a closed String."_view);
    return False;
  }

  Core::View::Bytes parsed_abi = quoted.slice(1, quoted.get_size() - 2);
  if (parsed_abi != "C"_view ||
      (host.get_abi() && *host.get_abi() != parsed_abi)) {
    cursor.create_token_error(
        abi_token,
        parsed_abi != "C"_view
            ? "Foreign supports only the exact `\"C\"` ABI selector."_view
            : "Foreign blocks in one source must use the same ABI."_view);
    return False;
  }
  Core::View::Bytes retained_abi =
      host.get_abi() ? *host.get_abi() : parsed_abi;
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart,
      "Foreign blocks require `{` before their declarations."_view));

  Memory::Managed::Vector<Tetrodotoxin::Source::Reference<Language::Foreign::State>>
      states(cursor.get_arena());
  Memory::Managed::Vector<Tetrodotoxin::Source::Reference<Language::Foreign::Function>>
      functions(cursor.get_arena());
  Memory::Managed::Vector<Tetrodotoxin::Source::Reference<Abstract>> declarations(
      cursor.get_arena());
  while (!cursor.matches(Code::Type::ScopeEnd)) {
    if (cursor.matches(Code::Type::Terminal)) {
      cursor.create_token_error(
          "Foreign block reached the end of source before `}`."_view);
      return False;
    }

    const Tetrodotoxin::Source::Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    Code::Type visibility = cursor.get_code().get_type();
    if (visibility != Code::Type::Public && visibility != Code::Type::Private &&
        visibility != Code::Type::Expose) {
      cursor.create_token_error(
          "Foreign declarations require authored visibility."_view);
      return False;
    }

    Code::Type qualifier = cursor.peek(1).get_code().get_type();
    if (qualifier == Code::Type::State || qualifier == Code::Type::Const) {
      auto state = parse_state(host, cursor, documentation, retained_abi);
      BAIL_IF(!state);
      Core::Option<const Language::Foreign::State&> duplicate;
      for (const auto& retained : host.get_states()) {
        if (retained.get().get_name() == state->get_name()) {
          duplicate = retained.get();
          break;
        }
      }
      if (!duplicate) {
        for (const auto& retained : states.get_view()) {
          if (retained.get().get_name() == state->get_name()) {
            duplicate = retained.get();
            break;
          }
        }
      }
      if (duplicate &&
          !has_same_declaration(*duplicate, *state, cursor.get_source_text())) {
        cursor.create_expression_error(
            state->get_anchor(),
            "Repeated Foreign State changes its declaration."_view,
            "Repeat the exact declaration or choose another symbol."_view);
        return False;
      }
      if (!duplicate) {
        states.insert(*state);
        declarations.insert(*state);
      }
      continue;
    }

    if (qualifier == Code::Type::Func) {
      auto function = parse_function(host, cursor, documentation, retained_abi);
      BAIL_IF(!function);
      Core::Option<const Language::Foreign::Function&> duplicate;
      for (const auto& retained : host.get_functions()) {
        if (retained.get().get_name() == function->get_name()) {
          duplicate = retained.get();
          break;
        }
      }
      if (!duplicate) {
        for (const auto& retained : functions.get_view()) {
          if (retained.get().get_name() == function->get_name()) {
            duplicate = retained.get();
            break;
          }
        }
      }
      if (duplicate && !has_same_declaration(
                           *duplicate, *function, cursor.get_source_text())) {
        cursor.create_expression_error(
            function->get_anchor(),
            "Repeated Foreign Function changes its declaration."_view,
            "Repeat the exact declaration or choose another symbol."_view);
        return False;
      }
      if (!duplicate) {
        functions.insert(*function);
        declarations.insert(*function);
      }
      continue;
    }

    cursor.create_token_error(
        cursor.peek(1),
        "Foreign declarations require `state`, `const`, or `func`."_view);
    return False;
  }

  BAIL_IF(!cursor.require(
      Code::Type::ScopeEnd,
      "Foreign blocks require `}` after their declarations."_view));
  BAIL_IF(!host.retain_block(
      block_documentation, retained_abi, states.get_view(),
      functions.get_view(), declarations.get_view()));
  for (const auto& state : states.get_view()) {
    const auto name = state.get().get_definition().get_authored().get_name();
    cursor.get_associations().create(
        Anchor::create(Span(name)), state.get());
  }

  for (const auto& function : functions.get_view()) {
    const auto name = function.get().get_definition().get_authored().get_name();
    cursor.get_associations().create(
        Anchor::create(Span(name)), function.get());
  }

  return True;
}
