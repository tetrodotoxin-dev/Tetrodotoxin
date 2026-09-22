// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/graph_text.hpp"

#include "perimortem/core/algorithm/sort.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/source/constant.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/callable.hpp"
#include "tetrodotoxin/source/type.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;

class GraphConceptEdge {
 public:
  constexpr GraphConceptEdge(Core::View::Bytes name, const Abstract& target)
      : name(name), target(target) {}

  // Provide an operator > for sorting edges.
  auto operator>(const GraphConceptEdge& other) const -> Bool;

  Core::View::Bytes name;
  Reference<const Abstract> target;
};

class GraphNode {
 public:
  constexpr explicit GraphNode(const Abstract& abstract) : abstract(abstract) {}

  Reference<const Abstract> abstract;
  Memory::Dynamic::Vector<GraphConceptEdge> concepts;
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

auto GraphConceptEdge::operator>(const GraphConceptEdge& other) const -> Bool {
  const S32 order = compare_bytes(name, other.name);
  if (order != 0) {
    return order > 0;
  }

  return compare_bytes(target.get().get_name(), other.target.get().get_name()) >
         0;
}

static auto find_node(
    Core::View::Vector<GraphNode> nodes,
    const Abstract& abstract) -> Core::Option<Count> {
  for (Count index = 0; index < nodes.get_size(); index++) {
    if (&nodes.get_data()[index].abstract.get() == &abstract) {
      return index;
    }
  }
  return {};
}

static auto retain_node(
    Memory::Dynamic::Vector<GraphNode>& nodes,
    const Abstract& abstract) -> Count {
  auto index = nodes.get_view().find(
      [&](const GraphNode& node) { return &node.abstract.get() == &abstract; });
  if (index != -1) {
    return index;
  }

  Count id = nodes.get_size();
  nodes.emplace(GraphNode(abstract));
  return id;
}

static auto retain_layout(
    Memory::Dynamic::Vector<GraphNode>& nodes,
    const Layout& layout) -> void {
  for (Count index = 0; index < layout.get_size(); index++) {
    auto selected = layout.get_abstract(index);
    if (selected) {
      retain_node(nodes, *selected);
    }
  }
}

static auto explore(
    Memory::Allocator::Arena& arena,
    Memory::Dynamic::Vector<GraphNode>& nodes) -> void {
  for (Count index = 0; index < nodes.get_size(); index++) {
    const Abstract& abstract = nodes[index].abstract.get();
    const Abstract& resolved = abstract.resolve();
    retain_node(nodes, resolved);
    retain_node(
        nodes, resolved.is<Unknown>()
                   ? static_cast<const Abstract&>(Unknown::get_unknown())
                   : abstract.get_type());

    // Graph text sorts its edges after discovery. It retains only the names
    // needed for that output, since a provider can lend temporary names while
    // visiting its representation.
    Memory::Dynamic::Vector<GraphConceptEdge> discovered;
    auto receive = [&](Core::View::Bytes name, const Abstract& target) {
      discovered.emplace(GraphConceptEdge(arena.proxy(name), target));
    };
    abstract.visit_concepts(Abstract::Visitor(receive));

    // Sort the concept edges by name order
    Core::Algorithm::sort(discovered.get_access());
    for (const GraphConceptEdge& edge : discovered.get_view()) {
      retain_node(nodes, edge.target.get());
    }

    auto type = abstract.select<Tetrodotoxin::Source::Type>();
    if (type) {
      retain_layout(nodes, type->get_layout());
    }

    // If the Abstract is a Callable then render out its layout.
    auto callable = abstract.select<Tetrodotoxin::Source::Callable>();
    if (callable) {
      retain_layout(nodes, callable->get_parameters());
      retain_layout(nodes, callable->get_results());
    }
    nodes[index].concepts =
        static_cast<Memory::Dynamic::Vector<GraphConceptEdge>&&>(discovered);
  }
}

static auto write_bytes(Memory::Dynamic::Bytes& output, Core::View::Bytes value)
    -> void {
  static constexpr U8 digits[] = "0123456789abcdef";
  output.append('x');
  for (Count index = 0; index < value.get_size(); index++) {
    U8 byte = value[index];
    output.append(digits[byte >> 4]);
    output.append(digits[byte & 0x0f]);
  }
}

static auto write_id(
    Serialization::Stream::Textual<Memory::Dynamic::Bytes>& output,
    Core::View::Vector<GraphNode> nodes,
    const Abstract& abstract) -> void {
  output << U64(*find_node(nodes, abstract));
}

static auto write_layout(
    Memory::Dynamic::Bytes& bytes,
    Serialization::Stream::Textual<Memory::Dynamic::Bytes>& output,
    Core::View::Vector<GraphNode> nodes,
    Core::View::Bytes role,
    const Layout& layout) -> void {
  output << "  layout "_view << role << " "_view << U64(layout.get_size())
         << "\n"_view;
  for (Count index = 0; index < layout.get_size(); index++) {
    output << "    entry "_view;
    auto name = layout.get_name(index);
    write_bytes(bytes, name ? *name : Core::View::Bytes());
    output << " "_view;
    layout.get_abstract(index).visit(
        [&]() { output << "none"_view; },
        [&](const Abstract& selected) { write_id(output, nodes, selected); });
    output << "\n"_view;
  }
}

static auto write_contracts(
    Serialization::Stream::Textual<Memory::Dynamic::Bytes>& output,
    const Abstract& abstract) -> void {
  output << "  contracts abstract"_view;
  if (abstract.is<Constant>()) {
    output << " constant"_view;
  }
  if (abstract.is<Tetrodotoxin::Source::Type>()) {
    output << " type"_view;
  }
  if (abstract.is<Tetrodotoxin::Source::Addressable>()) {
    output << " addressable"_view;
  }
  if (abstract.is<Tetrodotoxin::Source::Callable>()) {
    output << " callable"_view;
  }
  if (abstract.is<Unknown>()) {
    output << " unknown"_view;
  }
  if (abstract.is<None>()) {
    output << " none"_view;
  }
  output << "\n"_view;
}

static auto serialize(
    Core::View::Bytes source,
    const Abstract& dialect,
    const Abstract& root,
    Core::View::Vector<GraphNode> nodes) -> Memory::Dynamic::Bytes {
  Memory::Dynamic::Bytes bytes;
  Serialization::Stream::Textual<Memory::Dynamic::Bytes> output(bytes);
  output << "ttx.graph 1\nsource "_view;
  write_bytes(bytes, source);
  output << "\ndialect "_view;
  write_id(output, nodes, dialect);
  output << "\nroot "_view;
  write_id(output, nodes, root);
  output << "\nnodes "_view << U64(nodes.get_size()) << "\n"_view;

  for (Count id = 0; id < nodes.get_size(); id++) {
    const GraphNode& node = nodes.get_data()[id];
    const Abstract& abstract = node.abstract.get();
    output << "node "_view << U64(id) << "\n  name "_view;
    write_bytes(bytes, abstract.get_name());
    output << "\n"_view;
    write_contracts(output, abstract);
    output << "  resolve "_view;
    write_id(output, nodes, abstract.resolve());
    output << "\n  type "_view;
    const Abstract& resolved = abstract.resolve();
    write_id(
        output, nodes,
        resolved.is<Unknown>()
            ? static_cast<const Abstract&>(Unknown::get_unknown())
            : abstract.get_type());
    output << "\n  documentation "_view
           << U64(abstract.get_documentation().line_count()) << "\n"_view;
    for (Count line = 0; line < abstract.get_documentation().line_count();
         line++) {
      output << "    line "_view;
      write_bytes(bytes, abstract.get_documentation().get_line(line));
      output << "\n"_view;
    }
    output << "  concepts "_view << U64(node.concepts.get_size()) << "\n"_view;
    for (const GraphConceptEdge& edge : node.concepts.get_view()) {
      output << "    concept "_view;
      write_bytes(bytes, edge.name);
      output << " "_view;
      write_id(output, nodes, edge.target.get());
      output << "\n"_view;
    }
    auto type = abstract.select<Tetrodotoxin::Source::Type>();
    if (type) {
      write_layout(bytes, output, nodes, "type"_view, type->get_layout());
    }
    auto callable = abstract.select<Tetrodotoxin::Source::Callable>();
    if (callable) {
      write_layout(
          bytes, output, nodes, "parameters"_view, callable->get_parameters());
      write_layout(
          bytes, output, nodes, "results"_view, callable->get_results());
    }
    output << "end\n"_view;
  }
  return bytes;
}

auto Tetrodotoxin::Terminal::GraphText::write(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes source,
    const Abstract& dialect,
    const Abstract& root,
    const Abstract& graph) -> const Tetrodotoxin::Language::Product& {
  Memory::Dynamic::Vector<GraphNode> nodes;
  retain_node(nodes, graph);
  retain_node(nodes, dialect);
  retain_node(nodes, root);
  explore(arena, nodes);
  Memory::Dynamic::Bytes text =
      serialize(source, dialect, root, nodes.get_view());
  return Tetrodotoxin::Language::Product::create(
      arena, "graph.ttxg"_view, text.get_view());
}
