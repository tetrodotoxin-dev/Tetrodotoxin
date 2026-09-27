// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/app/language/monograph.hpp"
#include "tetrodotoxin/scene/language/monograph.hpp"
#include "tetrodotoxin/terminal/vulkan/products.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Terminal::Application {

// Generator projects one completed App Scene policy into the small native entry
// that composes Package symbols with the shared runtime. It emits no Scene
// model or target code of its own.
class Generator {
 public:
  class MemberBinding {
   public:
    constexpr MemberBinding(
        const Tetrodotoxin::Scene::Language::Monograph& scene,
        Perimortem::Core::View::Bytes route)
        : scene(scene), route(route) {}

    constexpr auto get_scene() const
        -> const Tetrodotoxin::Scene::Language::Monograph& {
      return scene.get();
    }

    constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
      return route;
    }

   private:
    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Scene::Language::Monograph>
        scene;
    Perimortem::Core::View::Bytes route;
  };

  Generator() = delete;

  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::App::Language::Monograph& app,
      Perimortem::Core::View::Bytes package,
      Perimortem::Core::View::Bytes artifact,
      Perimortem::Core::View::Vector<MemberBinding> members,
      const Tetrodotoxin::Source::Type& graphics_placement,
      Perimortem::Core::View::Vector<
          Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type>> graphics_types,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes>
          graphics_placements,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes>
          graphics_children,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes>
          graphics_drawables,
      Perimortem::Core::View::Vector<Tetrodotoxin::Terminal::Vulkan::Products>
          vulkan)
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;
};

}  // namespace Tetrodotoxin::Terminal::Application
