// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/abi/compiler.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/math.hpp"

#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/attribute.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/terminal/abi/c/header.hpp"
#include "tetrodotoxin/terminal/abi/cpp/header.hpp"
#include "tetrodotoxin/terminal/abi/representation/type.hpp"
#include "tetrodotoxin/terminal/abi/symbol.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library::Language;

class AbiGraphNode {
 public:
  constexpr AbiGraphNode(
      const Tetrodotoxin::Source::Abstract& semantic,
      Core::View::Bytes route,
      Count depth)
      : semantic(semantic), route(route), depth(depth) {}

  Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract> semantic;
  Core::View::Bytes route;
  Count depth;
};

static auto compare_bytes(Core::View::Bytes left, Core::View::Bytes right)
    -> S32 {
  Count size = Core::Math::min(left.get_size(), right.get_size());
  for (Count index = 0; index < size; index++) {
    if (left[index] != right[index]) {
      return left[index] < right[index] ? -1 : 1;
    }
  }
  if (left.get_size() == right.get_size()) {
    return 0;
  }
  return left.get_size() < right.get_size() ? -1 : 1;
}

static auto append_route(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes route,
    Core::View::Bytes name) -> Core::View::Bytes {
  if (name == "static"_view || name == "instance"_view) {
    return route;
  }
  Memory::Managed::Bytes appended(arena, route);
  if (!appended.get_view().is_empty()) {
    appended.concat("::"_view);
  }
  appended.concat(name);
  return appended.get_view();
}

static auto fail_interface(
    Tetrodotoxin::Source::Lexical::Errors& errors,
    Core::View::Bytes source_path,
    Core::View::Bytes source_text,
    const Tetrodotoxin::Language::Definition& definition,
    Core::View::Bytes message) -> Bool {
  Tetrodotoxin::Source::Lexical::Errors::Report report(
      errors, source_path, source_text, definition.get_authored().get_anchor());
  report << message;
  return False;
}

static auto get_attribute_text(
    const Tetrodotoxin::Language::Attribute& attribute)
    -> Core::Option<const Core::View::Bytes&> {
  const Core::View::Bytes* selected =
      attribute.get_value().find<Core::View::Bytes>();
  return selected ? Core::Option<const Core::View::Bytes&>(*selected)
                  : Core::Option<const Core::View::Bytes&>();
}

static auto select_symbol(
    Memory::Allocator::Arena& arena,
    const Function& function,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    Tetrodotoxin::Source::Lexical::Errors& errors,
    Core::View::Bytes source_path,
    Core::View::Bytes source_text) -> Core::Option<Core::View::Bytes> {
  Core::Option<Core::View::Bytes> abi;
  Core::Option<Core::View::Bytes> symbol;
  for (const Tetrodotoxin::Language::Attribute& attribute :
       function.get_definition().get_attributes()) {
    Core::View::Bytes key = attribute.get_key();
    if (key != "abi"_view && key != "symbol"_view) {
      continue;
    }
    auto text = get_attribute_text(attribute);
    if (!text) {
      fail_interface(
          errors, source_path, source_text, function.get_definition(),
          "Native interface Attributes require one string value."_view);
      return {};
    }
    if (key == "abi"_view) {
      if (abi) {
        fail_interface(
            errors, source_path, source_text, function.get_definition(),
            "A native interface accepts one ABI Attribute per Callable."_view);
        return {};
      }
      abi = *text;
    } else if (symbol) {
      fail_interface(
          errors, source_path, source_text, function.get_definition(),
          "A native interface accepts one symbol Attribute per Callable."_view);
      return {};
    } else {
      symbol = *text;
    }
  }

  if (abi && *abi != "C"_view) {
    fail_interface(
        errors, source_path, source_text, function.get_definition(),
        "The native interface supports the C ABI."_view);
    return {};
  }
  if (symbol &&
      (!abi || !Tetrodotoxin::Terminal::Abi::Symbol::validate(*symbol))) {
    fail_interface(
        errors, source_path, source_text, function.get_definition(),
        !abi ? "A native symbol override requires `@abi(\"C\")`."_view
             : "The native symbol is not a valid C identifier."_view);
    return {};
  }
  if (!symbol) {
    Tetrodotoxin::Terminal::Abi::Symbol generated(
        arena, function,
        function.declares_self()
            ? Tetrodotoxin::Terminal::Abi::Symbol::Kind::FunctionSelf
            : Tetrodotoxin::Terminal::Abi::Symbol::Kind::FunctionStatic,
        unit);
    return generated.get_view();
  }
  return *symbol;
}

static auto requests_native_interface(const Function& function) -> Bool {
  for (const Tetrodotoxin::Language::Attribute& attribute :
       function.get_definition().get_attributes()) {
    if (attribute.get_key() == "abi"_view ||
        attribute.get_key() == "symbol"_view) {
      return True;
    }
  }
  return False;
}

static auto collect_type(
    Memory::Allocator::Arena& arena,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Model::Type& type,
    Memory::Managed::Vector<Tetrodotoxin::Terminal::Abi::Export>& exports,
    Memory::Managed::Vector<Tetrodotoxin::Terminal::Abi::Publication>&
        publications,
    Tetrodotoxin::Source::Lexical::Errors& errors,
    Core::View::Bytes source_path,
    Core::View::Bytes source_text,
    Core::View::Vector<Tetrodotoxin::Source::Reference<const Model::Callable>> excluded)
    -> Bool {
  auto composite = type.select<Types::Composite>();
  if (composite) {
    for (const Tetrodotoxin::Source::Reference<Tetrodotoxin::Source::Abstract>& declaration :
         composite->get_declarations()) {
      auto nested = declaration.get().select<Model::Type>();
      if (nested && !collect_type(
                        arena, unit, *nested, exports, publications, errors,
                        source_path, source_text, excluded)) {
        return False;
      }

      auto function = declaration.get().select<Function>();
      Bool omitted = False;
      for (const Tetrodotoxin::Source::Reference<const Model::Callable>& candidate :
           excluded) {
        omitted |= &candidate.get() == &declaration.get();
      }
      if (function && !omitted &&
          (Tetrodotoxin::Terminal::Abi::is_publicly_reachable(
               function->get_definition()) ||
           requests_native_interface(*function))) {
        auto symbol = select_symbol(
            arena, *function, unit, errors, source_path, source_text);
        if (!symbol) {
          return False;
        }
        exports.insert(Tetrodotoxin::Terminal::Abi::Export(*function, *symbol));
        if (unit.is_package_member()) {
          publications.insert(
              Tetrodotoxin::Terminal::Abi::Publication(*function, *symbol));
        }
      }

      auto field = declaration.get().select<Field>();
      if (field && unit.is_package_member() &&
          field->get_writability() == Writability::Full &&
          Tetrodotoxin::Terminal::Abi::is_publicly_reachable(
              field->get_definition())) {
        Tetrodotoxin::Terminal::Abi::Symbol symbol(
            arena, *field, Tetrodotoxin::Terminal::Abi::Symbol::Kind::Address,
            unit);
        publications.insert(
            Tetrodotoxin::Terminal::Abi::Publication(
                *field, symbol.get_view()));
      }
    }
  }

  auto structure = type.select<Types::Structure>();
  if (structure && unit.is_package_member() &&
      !structure->get_layout().is_empty() &&
      structure->is_externally_reachable(*structure) &&
      structure->has_initialization_provider()) {
    Tetrodotoxin::Terminal::Abi::Symbol symbol(
        arena, *structure,
        Tetrodotoxin::Terminal::Abi::Symbol::Kind::Construction, unit);
    publications.insert(
        Tetrodotoxin::Terminal::Abi::Publication(
            *structure, symbol.get_view()));
  }
  return True;
}

auto Tetrodotoxin::Terminal::Abi::Compiler::compile_graph(
    Memory::Allocator::Arena& arena,
    const Library::Language::Monograph& monograph,
    const Tetrodotoxin::Source::Abstract& root,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    Tetrodotoxin::Source::Lexical::Errors& errors,
    Core::View::Bytes source_path,
    Core::View::Bytes source_text) const
    -> Core::Option<Tetrodotoxin::Terminal::Abi::Products> {
  Memory::Dynamic::Vector<AbiGraphNode> nodes;
  Memory::Dynamic::Vector<Count> pending;
  auto retain = [&](const Tetrodotoxin::Source::Abstract& semantic,
                    Core::View::Bytes route, Count depth) {
    for (Count index = 0; index < nodes.get_size(); index++) {
      AbiGraphNode& selected = nodes.get_access().get_data()[index];
      if (&selected.semantic.get() != &semantic) {
        continue;
      }
      Bool better =
          depth < selected.depth ||
          (depth == selected.depth && compare_bytes(route, selected.route) < 0);
      if (better) {
        selected.route = arena.proxy(route);
        selected.depth = depth;
        pending.emplace(Count(index));
      }
      return;
    }
    Count index = nodes.get_size();
    nodes.emplace(AbiGraphNode(semantic, arena.proxy(route), depth));
    pending.emplace(Count(index));
  };

  retain(root, {}, 0);
  Count next = 0;
  while (next < pending.get_size()) {
    Count index = pending.get_view().get_data()[next++];
    const AbiGraphNode& node = nodes.get_view().get_data()[index];
    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract> semantic_reference =
        node.semantic;
    Core::View::Bytes route = node.route;
    Count depth = node.depth;
    const Tetrodotoxin::Source::Abstract& semantic = semantic_reference.get();
    const Tetrodotoxin::Source::Abstract& resolved = semantic.resolve();
    if (!resolved.is<Tetrodotoxin::Source::Unknown>() &&
        !resolved.is<Tetrodotoxin::Source::None>()) {
      retain(resolved, route, depth);
    }

    auto receive = [&](Core::View::Bytes name,
                       const Tetrodotoxin::Source::Abstract& selected) {
      Bool authority = name == "static"_view || name == "instance"_view;
      retain(selected, append_route(arena, route, name),
             depth + (authority ? 0 : 1));
    };
    semantic.visit_concepts(Tetrodotoxin::Source::Abstract::Visitor(receive));
  }

  Memory::Managed::Vector<
      Tetrodotoxin::Source::Reference<const Library::Language::Model::Type>>
      roots(arena);
  Memory::Managed::Vector<Tetrodotoxin::Terminal::Abi::Unit::TypeBinding>
      bindings(arena);
  for (const AbiGraphNode& node : nodes.get_view()) {
    auto type =
        node.semantic.get().resolve().select<Library::Language::Model::Type>();
    if (!type || roots.get_view().contains([&](const auto& selected) {
          return &selected.get() == &*type;
        })) {
      continue;
    }
    roots.insert(*type);
    if (!node.route.is_empty()) {
      bindings.insert(
          Tetrodotoxin::Terminal::Abi::Unit::TypeBinding(
              *type, unit.get_package(), unit.get_member(), node.route));
    }
  }

  Tetrodotoxin::Terminal::Abi::Unit selected =
      unit.with_types(bindings.get_view());
  return compile(
      arena, monograph, selected, errors, source_path, source_text,
      roots.get_view());
}

auto Tetrodotoxin::Terminal::Abi::Compiler::compile(
    Memory::Allocator::Arena& arena,
    const Library::Language::Monograph& monograph,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    Tetrodotoxin::Source::Lexical::Errors& errors,
    Core::View::Bytes source_path,
    Core::View::Bytes source_text,
    Core::View::Vector<
        Tetrodotoxin::Source::Reference<const Library::Language::Model::Type>> roots,
    Core::View::Vector<
        Tetrodotoxin::Source::Reference<const Library::Language::Model::Callable>>
        excluded,
    Core::View::Vector<Tetrodotoxin::Terminal::Abi::Projection> projections)
    const -> Core::Option<Tetrodotoxin::Terminal::Abi::Products> {
  Memory::Managed::Vector<Tetrodotoxin::Terminal::Abi::Export> exports(arena);
  Memory::Managed::Vector<Tetrodotoxin::Terminal::Abi::Publication>
      publications(arena);
  if (roots.is_empty()) {
    if (!collect_type(
            arena, unit, monograph.get_source(), exports, publications, errors,
            source_path, source_text, excluded)) {
      return {};
    }
  } else {
    for (const Tetrodotoxin::Source::Reference<const Library::Language::Model::Type>&
             root : roots) {
      if (!collect_type(
              arena, unit, root.get(), exports, publications, errors,
              source_path, source_text, excluded)) {
        return {};
      }
    }
  }

  Tetrodotoxin::Terminal::Abi::Representation::Type types;
  auto c_header = Tetrodotoxin::Terminal::Abi::C::Header::create(
      arena, types, monograph, unit, exports.get_view(), roots);
  auto cpp_header = Tetrodotoxin::Terminal::Abi::Cpp::Header::create(
      arena, types, unit, exports.get_view());
  if (!c_header || !cpp_header) {
    return {};
  }
  return Tetrodotoxin::Terminal::Abi::Products(
      c_header->get_view(), cpp_header->get_header(), cpp_header->get_source(),
      exports.get_view(), publications.get_view(), projections);
}
