// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/serialization/json/blueprint.hpp"

#include "perimortem/memory/managed/vector.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;

auto Json::Blueprint::construct(Allocator::Arena& arena) const -> Json::Node {
  return visit(
      []() { return Json::Node(); },
      [](View::Bytes text) { return Json::Node(text); },
      [](S64 number) { return Json::Node(number); },
      [](R64 real) { return Json::Node(real); },
      [](Bool flag) { return Json::Node(flag); },
      [&](View::Vector<Json::Blueprint> compound) {
        const Bool is_object =
            !compound.is_empty() && !compound[0].get_name().is_empty();
        if (is_object) {
          Managed::Vector<Json::Node::Member> members(arena);
          for (Count i = 0; i < compound.get_size(); i++) {
            const auto& child =
                arena.construct<Json::Node>(compound[i].construct(arena));
            members.insert(Json::Node::Member(compound[i].get_name(), child));
          }

          return Json::Node(members);
        }

        Managed::Vector<Json::Node> nodes(arena);
        for (Count i = 0; i < compound.get_size(); i++) {
          nodes.insert(compound[i].construct(arena));
        }

        return Json::Node(nodes);
      },
      [](Json::Node node) { return node; });
}
