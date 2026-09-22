// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/access/call.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/builtin/fixed/access.hpp"
#include "tetrodotoxin/library/builtin/fixed/view.hpp"
#include "tetrodotoxin/library/builtin/view/is_empty.hpp"
#include "tetrodotoxin/library/builtin/view/size.hpp"
#include "tetrodotoxin/library/builtin/view/slice.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/flow/local.hpp"
#include "tetrodotoxin/library/language/flow/return.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness CallTests = {
  .name = "Tetrodotoxin::Library::Language::Access::Call"_view,
};

static auto find_call(const Language::Function& function)
    -> Option<const Language::Access::Call&> {
  auto body = function.get_body();
  BAIL_IF(!body);
  for (const Language::Statement& statement : body->get_statements()) {
    auto call = statement.get_root().select<Language::Access::Call>();
    if (call) {
      return *call;
    }
  }

  return {};
}

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, "CallTest"_view, "call.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto rejects_link(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (monograph || errors.is_empty()) {
    return False;
  }

  return retains_library_source(workspace, "CallTest"_view);
}

static auto rejects_interpretation(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (monograph || errors.is_empty()) {
    return False;
  }

  return retains_library_source(workspace, "CallTest"_view);
}

static auto find_type_callable(
    const Abstract& answer,
    View::Bytes name,
    Tetrodotoxin::Language::Visibility visibility =
        Tetrodotoxin::Language::Visibility::Private)
    -> Option<const Language::Model::Callable&> {
  auto type = answer.select<Language::Model::Type>();
  if (!type) {
    type = answer.resolve().select<Language::Model::Type>();
  }
  BAIL_IF(!type);
  for (const Reference<Abstract>& binding : type->get_callables(visibility)) {
    auto callable = binding.get().resolve().select<Language::Model::Callable>();
    if (binding.get().get_name() == name && callable) {
      return *callable;
    }
  }

  return {};
}

PERIMORTEM_UNIT_TEST(CallTests, builtin_callables) {
  static constexpr View::Bytes source =
      "// Contiguous built-in identities.\n"
      "dialect : Library;\n"
      "public Custom : struct {\n"
      "  public state value : U64;\n"
      "  public get_size : func = [self] -> U64 { return 9; }\n"
      "}\n"
      "public run : func = [] -> U64 {\n"
      "  state dense : Fixed[U64, 2] = (3, 4);\n"
      "  state borrowed : Access[U64] = dense -> get_access();\n"
      "  state viewed : View[U64];\n"
      "  viewed -> get_size();\n"
      "  viewed -> is_empty();\n"
      "  borrowed -> is_empty();\n"
      "  return borrowed -> get_size();\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  Option<const Language::Function&> run;
  for (const Reference<Abstract>& callable :
       monograph->get_source().get_callables()) {
    if (callable.get().get_name() == "run"_view &&
        callable.get().is<Language::Function>()) {
      run = static_cast<const Language::Function&>(callable.get());
      break;
    }
  }
  ASSERT(run && run->get_body());
  auto statements = run->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(7));

  const auto& dense = static_cast<const Language::Flow::Local&>(
      statements.get_data()[0].get_root());
  auto fixed_access = find_type_callable(
      dense.get_type(), "get_access"_view,
      Tetrodotoxin::Language::Visibility::Public);
  ASSERT(fixed_access);
  EXPECT(fixed_access->is<Builtin::Fixed::Access>());

  auto fixed_view = find_type_callable(
      dense.get_type(), "get_view"_view,
      Tetrodotoxin::Language::Visibility::Public);
  ASSERT(fixed_view);
  EXPECT(fixed_view->is<Builtin::Fixed::View>());

  const auto& borrowed = static_cast<const Language::Flow::Local&>(
      statements.get_data()[1].get_root());
  auto access_size = find_type_callable(
      borrowed.get_type(), "get_size"_view,
      Tetrodotoxin::Language::Visibility::Public);
  ASSERT(access_size);
  EXPECT(access_size->is<Builtin::View::Size>());

  auto access_empty = find_type_callable(
      borrowed.get_type(), "is_empty"_view,
      Tetrodotoxin::Language::Visibility::Public);
  ASSERT(access_empty);
  EXPECT(access_empty->is<Builtin::View::IsEmpty>());

  auto access_slice = find_type_callable(
      borrowed.get_type(), "slice"_view,
      Tetrodotoxin::Language::Visibility::Public);
  ASSERT(access_slice);
  EXPECT(access_slice->is<Builtin::View::Slice>());

  const auto& viewed = static_cast<const Language::Flow::Local&>(
      statements.get_data()[2].get_root());
  auto view_size = find_type_callable(
      viewed.get_type(), "get_size"_view,
      Tetrodotoxin::Language::Visibility::Public);
  ASSERT(view_size);
  EXPECT(view_size->is<Builtin::View::Size>());

  auto view_empty = find_type_callable(
      viewed.get_type(), "is_empty"_view,
      Tetrodotoxin::Language::Visibility::Public);
  ASSERT(view_empty);
  EXPECT(view_empty->is<Builtin::View::IsEmpty>());

  auto view_slice = find_type_callable(
      viewed.get_type(), "slice"_view,
      Tetrodotoxin::Language::Visibility::Public);
  ASSERT(view_slice);
  EXPECT(view_slice->is<Builtin::View::Slice>());

  ASSERT(statements.get_data()[3].get_root().is<Language::Access::Call>());
  const auto& view_call = static_cast<const Language::Access::Call&>(
      statements.get_data()[3].get_root());
  ASSERT(view_call.get_callable());
  EXPECT(&*view_call.get_callable() == &*view_size);
  ASSERT(statements.get_data()[4].get_root().is<Language::Access::Call>());
  const auto& view_empty_call = static_cast<const Language::Access::Call&>(
      statements.get_data()[4].get_root());
  ASSERT(view_empty_call.get_callable());
  EXPECT(&*view_empty_call.get_callable() == &*view_empty);
  ASSERT(statements.get_data()[5].get_root().is<Language::Access::Call>());
  const auto& access_empty_call = static_cast<const Language::Access::Call&>(
      statements.get_data()[5].get_root());
  ASSERT(access_empty_call.get_callable());
  EXPECT(&*access_empty_call.get_callable() == &*access_empty);
  ASSERT(borrowed.get_initializer());
  ASSERT(borrowed.get_initializer()->is_identity<Language::Access::Call>());
  const auto& get_access =
      static_cast<const Language::Access::Call&>(*borrowed.get_initializer());
  ASSERT(get_access.get_callable());
  EXPECT(get_access.get_callable()->is<Builtin::Fixed::Access>());
  EXPECT(get_access.get_type().resolve().is<Language::Types::Access>());

  const auto& returned = static_cast<const Language::Flow::Return&>(
      statements.get_data()[6].get_root());
  ASSERT(returned.get_pack().is_identity<Language::Access::Call>());
  const auto& get_size =
      static_cast<const Language::Access::Call&>(returned.get_pack());
  ASSERT(get_size.get_callable());
  EXPECT(get_size.get_callable()->is<Builtin::View::Size>());
  EXPECT_TEXT(get_size.get_type().resolve().get_name(), "U64"_view);

  const Abstract& custom_identity =
      monograph->get_source().resolve_concept("Custom"_view);
  auto custom = custom_identity.select<Language::Model::Type>();
  ASSERT(custom);
  auto custom_size = find_type_callable(
      *custom, "get_size"_view, Tetrodotoxin::Language::Visibility::Public);
  ASSERT(custom_size);
  EXPECT(custom_size->is<Language::Function>());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(CallTests, borrow_operations) {
  static constexpr View::Bytes source =
      "// Contiguous borrowing operations.\n"
      "dialect : Library;\n"
      "private run : func = [] -> U64 {\n"
      "  const label : View[U8] = \"Echo\" -> get_view();\n"
      "  const start : U64 = 1;\n"
      "  const count : U64 = 99;\n"
      "  const label_tail : View[U8] = "
      "label -> slice(.start = start, .count = count);\n"
      "  state dense : Fixed[U64, 3] = (1, 2, 3);\n"
      "  state viewed : View[U64] = dense -> get_view();\n"
      "  state writable : Access[U64] = dense -> get_access();\n"
      "  state tail : View[U64] = viewed -> slice(1, 99);\n"
      "  state write_tail : View[U64] = writable -> slice(9, 1);\n"
      "  return label -> get_size() + tail -> get_size() + "
      "write_tail -> get_size();\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  Option<const Language::Function&> run;
  for (const Reference<Abstract>& callable :
       monograph->get_source().get_callables()) {
    if (callable.get().get_name() == "run"_view &&
        callable.get().is<Language::Function>()) {
      run = static_cast<const Language::Function&>(callable.get());
      break;
    }
  }
  ASSERT(run && run->get_body());
  auto statements = run->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(10));

  const auto& label = static_cast<const Language::Flow::Local&>(
      statements.get_data()[0].get_root());
  ASSERT(label.get_initializer());
  auto get_view =
      label.get_initializer()->select_identity<Language::Access::Call>();
  ASSERT(get_view && get_view->get_callable());
  EXPECT(get_view->get_receiver()
             .get_type()
             .resolve()
             .is<Language::Types::Fixed>());
  EXPECT(get_view->get_callable()->is<Builtin::Fixed::View>());
  auto folded = label.get_constant();
  ASSERT(folded && folded->is_identity<Language::Constants::Bytes>());
  EXPECT_TEXT(
      static_cast<const Language::Constants::Bytes&>(*folded).get_value(),
      "Echo"_view);

  const auto& label_tail = static_cast<const Language::Flow::Local&>(
      statements.get_data()[3].get_root());
  folded = label_tail.get_constant();
  ASSERT(folded && folded->is_identity<Language::Constants::Bytes>());
  EXPECT_TEXT(
      static_cast<const Language::Constants::Bytes&>(*folded).get_value(),
      "cho"_view);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(CallTests, folded_bytes_borrow) {
  static constexpr View::Bytes source =
      "// Fitted bytes borrow.\n"
      "dialect : Library;\n"
      "private run : func = [] -> [] {\n"
      "  const repack : Fixed[U8, 9] = "
      "(\"file: \":[0, 6], \"Perimortem\":[0, 3]);\n"
      "  const view : View[U8] = repack -> get_view();\n"
      "  return;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  Option<const Language::Function&> run;
  for (const Reference<Abstract>& callable :
       monograph->get_source().get_callables()) {
    if (callable.get().get_name() == "run"_view &&
        callable.get().is<Language::Function>()) {
      run = static_cast<const Language::Function&>(callable.get());
      break;
    }
  }
  ASSERT(run && run->get_body());
  auto statements = run->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(3));

  const auto& repack = static_cast<const Language::Flow::Local&>(
      statements.get_data()[0].get_root());
  auto folded = repack.get_constant();
  ASSERT(folded && folded->is_identity<Language::Constants::Bytes>());
  EXPECT_TEXT(
      static_cast<const Language::Constants::Bytes&>(*folded).get_value(),
      "file: Per"_view);

  const auto& viewed = static_cast<const Language::Flow::Local&>(
      statements.get_data()[1].get_root());
  folded = viewed.get_constant();
  ASSERT(folded && folded->is_identity<Language::Constants::Bytes>());
  EXPECT_TEXT(
      static_cast<const Language::Constants::Bytes&>(*folded).get_value(),
      "file: Per"_view);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(CallTests, fixed_borrow_write) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Const local cannot grant Access.\ndialect : Library; private invalid : func = [] -> [] { const dense : Fixed[U64, 2] = (1, 2); state borrowed : Access[U64] = dense -> get_access(); return; }"_view,
    "// View remains read only.\ndialect : Library; private invalid : func = [] -> [] { state viewed : View[U64]; state borrowed : Access[U64] = viewed -> get_access(); return; }"_view,
  }};

  for (Count index = 0; index < sources.get_size(); index++) {
    EXPECT(rejects_link(sources[index]));
  }
}

static auto find_field(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Field&> {
  auto fields = composite.get_addressables();
  for (auto field = fields.begin(); field != fields.end(); ++field) {
    const Abstract& candidate = (*field).get();
    if (candidate.get_name() == name && candidate.is<Language::Field>()) {
      return static_cast<const Language::Field&>(candidate);
    }
  }

  return {};
}

static auto find_function(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Function&> {
  auto callables = composite.get_callables();
  for (auto callable = callables.begin(); callable != callables.end();
       ++callable) {
    const Abstract& candidate = (*callable).get();
    if (candidate.get_name() == name && candidate.is<Language::Function>()) {
      return static_cast<const Language::Function&>(candidate);
    }
  }

  return {};
}

PERIMORTEM_UNIT_TEST(CallTests, call_selection) {
  static constexpr View::Bytes source =
      "// Call selection test.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  private state positional := Packet -> choose(5, true,);\n"
      "  private named := Packet -> choose(.flag = true, .number = 5,);\n"
      "  public invoke : func = [self] -> Bool {\n"
      "    self -> choose(.flag = true,);\n"
      "    return true;\n"
      "  }\n"
      "  public choose : func = [\n"
      "    .number : U64, .flag : Bool,\n"
      "  ] -> U64 { return number; }\n"
      "  public choose : func = [self, .flag : Bool] -> Bool { return flag; }\n"
      "}\n"
      "public First : struct {\n"
      "  private seed : Later;\n"
      "  private copy := Factory -> identity(seed);\n"
      "}\n"
      "public Factory : struct {\n"
      "  public identity : func = [.value : Later] -> Later {\n"
      "    return value;\n"
      "  }\n"
      "}\n"
      "public Later : struct { public state value : Bool; }"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const Abstract& packet_identity =
      monograph->get_source().resolve_concept("Packet"_view);
  ASSERT(packet_identity.is<Language::Types::Structure>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  auto positional = find_field(packet, "positional"_view);
  auto named = find_field(packet, "named"_view);
  auto invoke = find_function(packet, "invoke"_view);
  ASSERT(positional);
  ASSERT(named);
  ASSERT(invoke);
  ASSERT(positional->get_initializer());
  ASSERT(positional->get_initializer()->is_identity<Language::Access::Call>());
  ASSERT(named->get_initializer());
  ASSERT(named->get_initializer()->is_identity<Language::Access::Call>());
  auto self_call = find_call(*invoke);
  ASSERT(self_call);

  const auto& positional_call = static_cast<const Language::Access::Call&>(
      *positional->get_initializer());
  const auto& named_call =
      static_cast<const Language::Access::Call&>(*named->get_initializer());
  ASSERT(positional_call.get_callable());
  ASSERT(named_call.get_callable());
  ASSERT(self_call->get_callable());
  auto self_entry = invoke->get_parameters().get_abstract(0);
  ASSERT(self_entry);
  auto self = self_entry->select<Tetrodotoxin::Source::Addressable>();
  ASSERT(self);
  const Abstract& u64 = monograph->resolve_concept("U64"_view);
  const Abstract& boolean = monograph->resolve_concept("Bool"_view);
  EXPECT(&self->resolve_concept("Packet"_view) == &None::get_none());
  EXPECT(
      &self->get_type()
           .resolve_concept("instance"_view)
           .resolve_concept("positional"_view) == &*positional);
  EXPECT(
      &self->get_type()
           .resolve_concept("instance"_view)
           .resolve_concept("choose"_view) == &*self_call->get_callable());
  EXPECT_NOT(positional_call.get_callable()->is_type_bound());
  EXPECT(self_call->get_callable()->is_type_bound(packet));
  EXPECT(&positional->get_type() == &u64);
  EXPECT(&named->get_type() == &u64);
  EXPECT(&positional_call.get_type() == &u64);
  EXPECT(&self_call->get_type() == &boolean);

  const Abstract& first_identity =
      monograph->get_source().resolve_concept("First"_view);
  const Abstract& later_identity =
      monograph->get_source().resolve_concept("Later"_view);
  ASSERT(first_identity.is<Language::Types::Structure>());
  ASSERT(later_identity.is<Language::Types::Structure>());
  auto copy = find_field(
      static_cast<const Language::Types::Structure&>(first_identity),
      "copy"_view);
  ASSERT(copy);
  EXPECT(&copy->get_type() == &later_identity);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(CallTests, invalid_invocation) {
  static constexpr Static::Vector<View::Bytes, 2> invalid_calls = {{
    "// Call mismatch test.\ndialect : Library;\n"
    "public Target : struct {\n"
    "  public use : func = [.value : U64] -> Bool { return true; }\n"
    "}\n"
    "private invalid := Target -> use(false);"_view,
    "// Call arity test.\ndialect : Library;\n"
    "public Target : struct {\n"
    "  public use : func = [.value : U64] -> Bool { return true; }\n"
    "}\n"
    "private invalid := Target -> use();"_view,
  }};

  for (Count i = 0; i < invalid_calls.get_size(); i++) {
    EXPECT(rejects_link(invalid_calls[i]));
  }
}

PERIMORTEM_UNIT_TEST(CallTests, duplicate_role) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Static Callable registration collision.\n"
    "dialect : Library;\n"
    "public Target : struct {\n"
    "  public use : func = [] -> Bool { return true; }\n"
    "  private use : func = [.value : U64] -> U64 {\n"
    "    return value;\n"
    "  }\n"
    "}"_view,
    "// Self Callable registration collision.\n"
    "dialect : Library;\n"
    "public Target : struct {\n"
    "  public use : func = [self] -> Bool { return true; }\n"
    "  private use : func = [self, .value : U64] -> U64 {\n"
    "    return value;\n"
    "  }\n"
    "}"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(CallTests, argument_parentheses) {
  EXPECT(rejects_interpretation(
      "// Missing Call argument Pack.\n"
      "dialect : Library;\n"
      "public Target : struct {\n"
      "  public use : func = [] -> Bool { return true; }\n"
      "}\n"
      "private invalid := Target -> use;"_view));
}

PERIMORTEM_UNIT_TEST(CallTests, host_authority) {
  static constexpr View::Bytes accepted =
      "// Hosted private Call test.\n"
      "dialect : Library;\n"
      "public Vault : struct {\n"
      "  private secret : func = [] -> Bool { return true; }\n"
      "  public open : func = [] -> Bool { return true; }\n"
      "  public Nested : struct {\n"
      "    private observed := Vault -> secret();\n"
      "  }\n"
      "}\n"
      "public VaultAlias : alias = Vault;\n"
      "private public_alias := VaultAlias -> open();"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, accepted);
  ASSERT(monograph);

  const Abstract& vault_identity =
      monograph->get_source().resolve_concept("Vault"_view);
  ASSERT(vault_identity.is<Language::Types::Structure>());
  const auto& vault =
      static_cast<const Language::Types::Structure&>(vault_identity);
  const Abstract& nested_identity = vault.resolve_concept("Nested"_view);
  ASSERT(nested_identity.is<Language::Types::Structure>());
  const auto& nested =
      static_cast<const Language::Types::Structure&>(nested_identity);
  auto observed = find_field(nested, "observed"_view);
  auto public_alias = find_field(monograph->get_source(), "public_alias"_view);
  ASSERT(observed);
  ASSERT(public_alias);
  const Abstract& boolean = monograph->resolve_concept("Bool"_view);
  EXPECT(&observed->get_type() == &boolean);
  EXPECT(&public_alias->get_type() == &boolean);
  EXPECT(errors.is_empty());

  EXPECT(rejects_link(
      "// Alias private Call test.\ndialect : Library;\n"
      "public Vault : struct {\n"
      "  private secret : func = [] -> Bool { return true; }\n"
      "}\n"
      "public VaultAlias : alias = Vault;\n"
      "private denied := VaultAlias -> secret();"_view));
}

PERIMORTEM_UNIT_TEST(CallTests, call_result_access) {
  static constexpr View::Bytes source =
      "// Call result flow test.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  public state value : U64; public state flag : Bool;\n"
      "  public self_none : func = [self] -> [] {}\n"
      "  public self_one : func = [self] -> Bool { return true; }\n"
      "}\n"
      "public Results : struct {\n"
      "  public none : func = [] -> [] {}\n"
      "  public one : func = [] -> Bool { return true; }\n"
      "  public many : func = [.packet : Packet] -> [U64, Bool] {\n"
      "    return packet.[value, flag];\n"
      "  }\n"
      "  public identity : func = [.packet : Packet] -> Packet {\n"
      "    return packet;\n"
      "  }\n"
      "}\n"
      "private seed : Packet;\n"
      "private selected := seed.value;\n"
      "private observe : func = [] -> [] {\n"
      "  Results -> none();\n"
      "  Results -> one();\n"
      "  Results -> many(seed);\n"
      "  seed -> self_none();\n"
      "  seed -> self_one();\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto observe = find_function(monograph->get_source(), "observe"_view);
  ASSERT(observe);
  auto body = observe->get_body();
  ASSERT(body);
  auto statements = body->get_statements();
  ASSERT_EQ(statements.get_size(), Count(5));
  for (Count i = 0; i < statements.get_size(); i++) {
    ASSERT(statements.get_data()[i].get_root().is<Language::Access::Call>());
  }

  const auto& none = static_cast<const Language::Access::Call&>(
      statements.get_data()[0].get_root());
  const auto& one = static_cast<const Language::Access::Call&>(
      statements.get_data()[1].get_root());
  const auto& many = static_cast<const Language::Access::Call&>(
      statements.get_data()[2].get_root());
  const auto& self_none = static_cast<const Language::Access::Call&>(
      statements.get_data()[3].get_root());
  const auto& self_one = static_cast<const Language::Access::Call&>(
      statements.get_data()[4].get_root());
  EXPECT(none.get_layout().is_empty());
  EXPECT(&none.get_type() == &Unknown::get_unknown());
  EXPECT_EQ(one.get_layout().get_size(), Count(1));
  EXPECT(&one.get_type() == &monograph->resolve_concept("Bool"_view));
  EXPECT_EQ(many.get_layout().get_size(), Count(2));
  EXPECT(&many.get_type() == &Unknown::get_unknown());
  EXPECT(&many.get_value_type(0) == &monograph->resolve_concept("U64"_view));
  EXPECT(&many.get_value_type(1) == &monograph->resolve_concept("Bool"_view));
  EXPECT(&many.get_value_type(2) == &Unknown::get_unknown());
  EXPECT(self_none.get_layout().is_empty());
  EXPECT_EQ(self_one.get_layout().get_size(), Count(1));
  ASSERT(self_none.get_callable());
  ASSERT(self_one.get_callable());
  EXPECT(self_none.get_callable()->is_type_bound());
  EXPECT(self_one.get_callable()->is_type_bound());

  ASSERT(many.get_callable());
  const Layout& many_results = many.get_callable()->get_results();
  EXPECT(many.get_layout().fits(many_results));
  for (Count index = 0; index < many.get_layout().get_size(); index++) {
    EXPECT(
        many.get_layout()
            .get_fitted(many_results, index)
            .visit(
                [&](const Abstract& fitted) { return Bool(&fitted == &many); },
                [](Layout::Errors) { return False; }));
  }

  auto selected = find_field(monograph->get_source(), "selected"_view);
  ASSERT(selected);
  ASSERT(selected->get_initializer());
  ASSERT(selected->get_initializer()->is_identity<Language::Access::Address>());
  const auto& address = static_cast<const Language::Access::Address&>(
      *selected->get_initializer());
  EXPECT(address.get_receiver().get_result().is<Tetrodotoxin::Source::Addressable>());
  EXPECT(&selected->get_type() == &monograph->resolve_concept("U64"_view));
  EXPECT(errors.is_empty());

  static constexpr View::Bytes invalid_source =
      "// Computed result Address test.\n"
      "dialect : Library;\n"
      "public Packet : struct { public value : U64; }\n"
      "public Results : struct {\n"
      "  public identity : func = [.packet : Packet] -> Packet {\n"
      "    return packet;\n"
      "  }\n"
      "}\n"
      "private seed : Packet;\n"
      "private invalid := Results -> identity(seed).value;"_view;
  Tetrodotoxin::Library::Dialect invalid_workspace_toolchain_library;
  auto invalid_workspace_toolchain =
      Validation::create_library_toolchain(invalid_workspace_toolchain_library);
  Workspace invalid_workspace(*invalid_workspace_toolchain);
  Errors invalid_errors;
  auto invalid_monograph =
      interpret(invalid_workspace, invalid_errors, invalid_source);
  EXPECT_NOT(invalid_monograph);
  EXPECT(retains_library_source(invalid_workspace, "CallTest"_view));
  EXPECT(!invalid_errors.is_empty());
}
