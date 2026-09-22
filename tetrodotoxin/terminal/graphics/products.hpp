// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/scene/language/monograph.hpp"
#include "tetrodotoxin/source/reference.hpp"

namespace Tetrodotoxin::Terminal::Graphics {

// Products keeps the target behavior selected for one completed Scene. Each
// hosted entry points to the real Field that supplied it and optionally one
// Fixed element in that Field. Its type index selects one configured set of
// runtime Interfaces. LLVM can compile those facts into access behavior
// without copying a Scene tree or publishing semantic state.
class Products {
 public:
  class Hosted {
   public:
    constexpr Hosted(
        const Tetrodotoxin::Library::Language::Field& field,
        Count type_index,
        Perimortem::Core::Option<Count> element_index = {})
        : field(field),
          type_index(type_index),
          element_index(element_index) {}

    constexpr auto get_field() const
        -> const Tetrodotoxin::Library::Language::Field& {
      return field.get();
    }

    constexpr auto get_type_index() const -> Count { return type_index; }

    constexpr auto get_element_index() const
        -> Perimortem::Core::Option<Count> {
      return element_index;
    }

   private:
    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Library::Language::Field> field;
    Count type_index;
    Perimortem::Core::Option<Count> element_index;
  };

  constexpr Products(
      const Tetrodotoxin::Scene::Language::Monograph& scene,
      Perimortem::Core::View::Vector<Hosted> hosted)
      : scene(scene), hosted(hosted) {}

  constexpr auto get_scene() const
      -> const Tetrodotoxin::Scene::Language::Monograph& {
    return scene.get();
  }

  constexpr auto get_hosted() const -> Perimortem::Core::View::Vector<Hosted> {
    return hosted;
  }

 private:
  Tetrodotoxin::Source::Reference<const Tetrodotoxin::Scene::Language::Monograph> scene;
  Perimortem::Core::View::Vector<Hosted> hosted;
};

}  // namespace Tetrodotoxin::Terminal::Graphics
