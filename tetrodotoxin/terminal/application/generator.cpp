// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/application/generator.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/app/language/scene.hpp"
#include "tetrodotoxin/app/language/transition.hpp"
#include "tetrodotoxin/scene/language/lifecycle.hpp"
#include "tetrodotoxin/terminal/abi/symbol.hpp"
#include "tetrodotoxin/terminal/abi/unit.hpp"
#include "tetrodotoxin/terminal/graphics/compiler.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

struct ApplicationSceneSelection {
  Tetrodotoxin::Source::Reference<const Scene::Language::Monograph> scene;
  Core::View::Bytes route;
};

static auto find_scene(
    Core::View::Vector<ApplicationSceneSelection> scenes,
    const Scene::Language::Monograph& scene) -> Core::Option<Count> {
  for (Count index = 0; index < scenes.get_size(); index++) {
    if (&scenes.get_data()[index].scene.get() == &scene) {
      return index;
    }
  }
  return {};
}

static auto retain_scene(
    Memory::Managed::Vector<ApplicationSceneSelection>& scenes,
    const Scene::Language::Monograph& scene,
    Core::View::Bytes route) -> Bool {
  auto selected = find_scene(scenes.get_view(), scene);
  if (selected) {
    return scenes[*selected].route == route;
  }
  scenes.insert(ApplicationSceneSelection{scene, route});
  return True;
}

static auto find_member_route(
    Core::View::Vector<Terminal::Application::Generator::MemberBinding> members,
    const Scene::Language::Monograph& scene)
    -> Core::Option<Core::View::Bytes> {
  for (const Terminal::Application::Generator::MemberBinding& member :
       members) {
    if (&member.get_scene() == &scene) {
      return member.get_route();
    }
  }
  return {};
}

static auto write_symbol_declaration(
    Serialization::Stream::Textual<Memory::Dynamic::Bytes>& output,
    Core::View::Bytes result,
    Core::View::Bytes symbol,
    Core::View::Bytes parameters) -> void {
  output << "extern \"C\" "_view << result << " "_view << symbol << "("_view
         << parameters << ");\n"_view;
}

static auto function_symbol(
    Memory::Allocator::Arena& arena,
    const Library::Language::Function& function,
    Terminal::Abi::Unit unit) -> Core::View::Bytes {
  Terminal::Abi::Symbol symbol(
      arena, function, Terminal::Abi::Symbol::Kind::FunctionSelf, unit);
  return symbol.get_view();
}

static auto lifecycle_symbol(
    Memory::Allocator::Arena& arena,
    const Scene::Language::Monograph& scene,
    Scene::Language::Lifecycle role,
    Terminal::Abi::Unit unit) -> Core::Option<Core::View::Bytes> {
  auto function = scene.get_lifecycle(role);
  return function ? Core::Option<Core::View::Bytes>(
                        function_symbol(arena, *function, unit))
                  : Core::Option<Core::View::Bytes>();
}

static auto graphics_children_symbol(
    Memory::Allocator::Arena& arena,
    const Scene::Language::Monograph& scene,
    Terminal::Abi::Unit unit) -> Core::View::Bytes {
  Terminal::Abi::Symbol symbol(
      arena, scene.get_instance(),
      Terminal::Abi::Symbol::Kind::GraphicsChildren, unit);
  return symbol.get_view();
}

static auto write_callback(
    Serialization::Stream::Textual<Memory::Dynamic::Bytes>& output,
    Core::Option<Core::View::Bytes> symbol) -> void {
  if (symbol) {
    output << "&"_view << *symbol;
  } else {
    output << "nullptr"_view;
  }
}

auto Terminal::Application::Generator::create(
    Memory::Allocator::Arena& arena,
    const App::Language::Monograph& app,
    Core::View::Bytes package,
    Core::View::Bytes artifact,
    Core::View::Vector<MemberBinding> members,
    const Tetrodotoxin::Source::Type& graphics_placement,
    Core::View::Vector<Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type>>
        graphics_types,
    Core::View::Vector<Core::View::Bytes> graphics_placements,
    Core::View::Vector<Core::View::Bytes> graphics_children,
    Core::View::Vector<Core::View::Bytes> graphics_drawables,
    Core::View::Vector<Terminal::Vulkan::Products> vulkan)
    -> Core::Option<Memory::Dynamic::Bytes> {
  auto policy = app.get_scene();
  auto windowed = app.get_runtime().get_windowed();
  BAIL_IF(
      !policy || !windowed || package.is_empty() || artifact.is_empty() ||
      graphics_placements.get_size() != graphics_types.get_size() ||
      graphics_children.get_size() != graphics_types.get_size() ||
      graphics_drawables.get_size() != graphics_types.get_size() ||
      vulkan.is_empty());
  for (Count index = 0; index < graphics_types.get_size(); index++) {
    BAIL_IF(
        (graphics_placements[index].is_empty() &&
         graphics_children[index].is_empty() &&
         graphics_drawables[index].is_empty()) ||
        (!graphics_placements[index].is_empty() &&
         !Terminal::Abi::Symbol::validate(graphics_placements[index])) ||
        (!graphics_children[index].is_empty() &&
         !Terminal::Abi::Symbol::validate(graphics_children[index])) ||
        (!graphics_drawables[index].is_empty() &&
         !Terminal::Abi::Symbol::validate(graphics_drawables[index])));
  }
  auto initial = policy->get_initial_scene();
  BAIL_IF(!initial);
  auto initial_route = find_member_route(members, *initial);
  BAIL_IF(!initial_route);

  Memory::Managed::Vector<ApplicationSceneSelection> scenes(arena);
  BAIL_IF(!retain_scene(scenes, *initial, *initial_route));
  for (const Tetrodotoxin::Source::Reference<App::Language::Transition>& retained :
       policy->get_transitions()) {
    const App::Language::Transition& transition = retained.get();
    auto source = transition.get_source_scene();
    auto source_route = source ? find_member_route(members, *source)
                               : Core::Option<Core::View::Bytes>();
    BAIL_IF(
        !source || !source_route ||
        !retain_scene(scenes, *source, *source_route));
    auto destination = transition.get_destination_scene();
    if (destination) {
      auto destination_route = find_member_route(members, *destination);
      BAIL_IF(
          !destination_route ||
          !retain_scene(scenes, *destination, *destination_route));
    }
  }

  Memory::Managed::Vector<Terminal::Graphics::Products> graphics(arena);
  Terminal::Graphics::Compiler graphics_compiler;
  for (const ApplicationSceneSelection& selected : scenes.get_view()) {
    auto product = graphics_compiler.compile(
        arena, selected.scene.get(), graphics_placement, graphics_types);
    BAIL_IF(!product);
    graphics.insert(*product);
  }

  Memory::Dynamic::Bytes source;
  Serialization::Stream::Textual<Memory::Dynamic::Bytes> output(source);
  output << "// # Tetrodotoxin\n"_view
         << "// Copyright (c) 2023-present Matt Kaes and contributors\n\n"_view
         << "#include \"perimortem/core/data.hpp\"\n"_view
         << "#include \"perimortem/core/null_terminated.hpp\"\n"_view
         << "#include \"perimortem/vulkan/description/program.hpp\"\n"_view
         << "#include \"tetrodotoxin/runtime/application/runner.hpp\"\n\n"_view;

  for (const Terminal::Vulkan::Products& product : vulkan) {
    Memory::Managed::Bytes shader_end(arena, product.get_symbol());
    shader_end.concat("_end"_view);
    output << "extern \"C\" const U8 "_view << product.get_symbol()
           << "[];\nextern \"C\" const U8 "_view << shader_end.get_view()
           << "[];\n"_view;
  }
  output << "\n"_view;
  for (Count index = 0; index < graphics_types.get_size(); index++) {
    if (!graphics_placements[index].is_empty()) {
      output << "extern \"C\" const "
                "Tetrodotoxin::Graphics::Runtime::Placement2D* "_view
             << graphics_placements[index] << "();\n"_view;
    }
    if (!graphics_children[index].is_empty()) {
      output << "extern \"C\" const "
                "Tetrodotoxin::Graphics::Runtime::Children2D* "_view
             << graphics_children[index] << "();\n"_view;
    }
    if (!graphics_drawables[index].is_empty()) {
      output << "extern \"C\" const "
                "Tetrodotoxin::Graphics::Runtime::Drawable2D* "_view
             << graphics_drawables[index] << "();\n"_view;
    }
  }
  output << "\n"_view;

  for (const ApplicationSceneSelection& selected : scenes.get_view()) {
    Terminal::Abi::Unit unit(package, selected.route, artifact);
    Terminal::Abi::Symbol construction(
        arena, selected.scene.get().get_instance(),
        Terminal::Abi::Symbol::Kind::Construction, unit);
    write_symbol_declaration(
        output, "void*"_view, construction.get_view(), "void"_view);
    auto prepare = lifecycle_symbol(
        arena, selected.scene.get(), Scene::Language::Lifecycle::Prepare, unit);
    auto pause = lifecycle_symbol(
        arena, selected.scene.get(), Scene::Language::Lifecycle::Pause, unit);
    auto resume = lifecycle_symbol(
        arena, selected.scene.get(), Scene::Language::Lifecycle::Resume, unit);
    auto update = lifecycle_symbol(
        arena, selected.scene.get(), Scene::Language::Lifecycle::Update, unit);
    auto release = lifecycle_symbol(
        arena, selected.scene.get(), Scene::Language::Lifecycle::Release, unit);
    BAIL_IF(!prepare || !update || !release);
    write_symbol_declaration(output, "void"_view, *prepare, "void**"_view);
    if (pause) {
      write_symbol_declaration(output, "void"_view, *pause, "void**"_view);
    }
    if (resume) {
      write_symbol_declaration(output, "void"_view, *resume, "void**"_view);
    }
    write_symbol_declaration(
        output, "void"_view, *update, "void**, double"_view);
    write_symbol_declaration(output, "void"_view, *release, "void**"_view);
    write_symbol_declaration(
        output, "Count"_view,
        graphics_children_symbol(arena, selected.scene.get(), unit),
        "void*, Count, void**"_view);
  }

  for (const Tetrodotoxin::Source::Reference<App::Language::Transition>& retained :
       policy->get_transitions()) {
    const App::Language::Transition& transition = retained.get();
    auto source_scene = transition.get_source_scene();
    auto signal = transition.get_signal();
    BAIL_IF(!source_scene || !signal);
    auto source_index = find_scene(scenes.get_view(), *source_scene);
    BAIL_IF(!source_index);
    Terminal::Abi::Unit unit(package, scenes[*source_index].route, artifact);
    Terminal::Abi::Symbol token(
        arena, *signal, Terminal::Abi::Symbol::Kind::ReadOnly, unit);
    output << "extern \"C\" const U8 "_view << token.get_view() << ";\n"_view;
  }

  for (Count product_index = 0; product_index < vulkan.get_size();
       product_index++) {
    const Terminal::Vulkan::Products& product =
        vulkan.get_data()[product_index];
    Memory::Managed::Bytes shader_end(arena, product.get_symbol());
    shader_end.concat("_end"_view);
    output << "\nstatic const Perimortem::Vulkan::Description::Module "
              "application_modules_"_view
           << product_index << "[] = {\n"_view;
    for (const Terminal::Vulkan::Products::Entry& entry :
         product.get_entries()) {
      output << "  {Perimortem::Vulkan::Description::Stage::"_view
             << (entry.stage == Terminal::Vulkan::Products::Stage::Vertex
                     ? "Vertex"_view
                     : "Pixel"_view)
             << ", Perimortem::Core::View::Vector<U32>("
                "Perimortem::Core::Data::cast<const U32>("_view
             << product.get_symbol() << "), Count("_view
             << shader_end.get_view() << " - "_view << product.get_symbol()
             << ") / sizeof(U32)), \""_view << entry.name << "\"_view},\n"_view;
    }
    output << "};\n\nstatic const Perimortem::Vulkan::Description::Stage "
              "application_push_stages_"_view
           << product_index << "[] = {\n"_view;
    for (const Terminal::Vulkan::Products::Entry& entry :
         product.get_entries()) {
      output << "  Perimortem::Vulkan::Description::Stage::"_view
             << (entry.stage == Terminal::Vulkan::Products::Stage::Vertex
                     ? "Vertex"_view
                     : "Pixel"_view)
             << ",\n"_view;
    }
    output
        << "};\n\nstatic const Perimortem::Vulkan::Description::HostInputRange "
           "application_host_ranges_"_view
        << product_index << "[] = {\n  {0, "_view << product.get_host_size()
        << ", Perimortem::Core::View::Vector<"
           "Perimortem::Vulkan::Description::Stage>(application_push_stages_"_view
        << product_index << ", "_view << product.get_entries().get_size()
        << ")},\n};\n\n"_view
        << "static const Perimortem::Vulkan::Description::DescriptorBinding "
           "application_descriptors_"_view
        << product_index << "[] = {\n"_view;
    for (const Terminal::Vulkan::Products::Descriptor& descriptor :
         product.get_descriptors()) {
      output
          << "  {\""_view << descriptor.name << "\"_view, "_view
          << descriptor.set << ", "_view << descriptor.slot
          << ", Perimortem::Vulkan::Description::Resource::SampledTexture2D},\n"_view;
    }
    output << "};\n\nstatic const Perimortem::Vulkan::Description::HostField "
              "application_host_fields_"_view
           << product_index << "[] = {\n"_view;
    for (const Terminal::Vulkan::Products::HostField& field :
         product.get_host_fields()) {
      output << "  {\""_view << field.name << "\"_view, "_view << field.offset
             << ", "_view << field.size
             << ", Perimortem::Vulkan::Description::HostRole::"_view;
      switch (field.role) {
      case Terminal::Vulkan::Products::HostRole::Parameter:
        output << "Parameter"_view;
        break;
      case Terminal::Vulkan::Products::HostRole::TransformX:
        output << "TransformX"_view;
        break;
      case Terminal::Vulkan::Products::HostRole::TransformY:
        output << "TransformY"_view;
        break;
      }
      output << "},\n"_view;
    }
    output << "};\n\nstatic const Perimortem::Vulkan::Description::VertexInput "
              "application_vertex_inputs_"_view
           << product_index << "[] = {\n"_view;
    for (const Terminal::Vulkan::Products::VertexInput& input :
         product.get_vertex_inputs()) {
      output << "  {"_view << input.location << ", "_view << input.components
             << ", "_view << input.offset << ", "_view << input.stride
             << "},\n"_view;
    }
    output << "};\n"_view;
  }

  output << "\nstatic const Perimortem::Vulkan::Description::Program "
            "application_programs[] = {\n"_view;
  for (Count product_index = 0; product_index < vulkan.get_size();
       product_index++) {
    const Terminal::Vulkan::Products& product =
        vulkan.get_data()[product_index];
    output
        << "  {"_view << product.get_symbol()
        << ", Perimortem::Core::View::Vector<Perimortem::Vulkan::Description::Module>(application_modules_"_view
        << product_index << ", "_view << product.get_entries().get_size()
        << "), Perimortem::Core::View::Vector<Perimortem::Vulkan::Description::HostInputRange>(application_host_ranges_"_view
        << product_index
        << ", 1), Perimortem::Core::View::Vector<Perimortem::Vulkan::Description::DescriptorBinding>(application_descriptors_"_view
        << product_index << ", "_view << product.get_descriptors().get_size()
        << "), Perimortem::Core::View::Vector<Perimortem::Vulkan::Description::HostField>(application_host_fields_"_view
        << product_index << ", "_view << product.get_host_fields().get_size()
        << "), Perimortem::Core::View::Vector<Perimortem::Vulkan::Description::VertexInput>(application_vertex_inputs_"_view
        << product_index << ", "_view << product.get_vertex_inputs().get_size()
        << "), "_view << product.get_host_size() << ", "_view
        << product.get_parameters_offset() << ", "_view
        << product.get_parameters_size() << ", "_view
        << (product.needs_float64() ? "true"_view : "false"_view)
        << "},\n"_view;
  }
  output << "};\n"_view;

  output << "\nstatic const U8 application_title[] = {"_view;
  Core::View::Bytes title = windowed->get_title().visit(
      []() { return "Tetrodotoxin"_view; },
      [](Core::View::Bytes selected) { return selected; });
  for (Count index = 0; index < title.get_size(); index++) {
    output << U32(title[index]) << ", "_view;
  }
  output << "0};\n\n"_view
         << "static const Tetrodotoxin::Runtime::Application::Scene "
            "application_scenes[] = {\n"_view;
  for (const ApplicationSceneSelection& selected : scenes.get_view()) {
    Terminal::Abi::Unit unit(package, selected.route, artifact);
    Terminal::Abi::Symbol construction(
        arena, selected.scene.get().get_instance(),
        Terminal::Abi::Symbol::Kind::Construction, unit);
    output << "  {&"_view << construction.get_view() << ", "_view;
    write_callback(
        output, lifecycle_symbol(
                    arena, selected.scene.get(),
                    Scene::Language::Lifecycle::Prepare, unit));
    output << ", "_view;
    write_callback(
        output, lifecycle_symbol(
                    arena, selected.scene.get(),
                    Scene::Language::Lifecycle::Pause, unit));
    output << ", "_view;
    write_callback(
        output, lifecycle_symbol(
                    arena, selected.scene.get(),
                    Scene::Language::Lifecycle::Resume, unit));
    output << ", "_view;
    write_callback(
        output, lifecycle_symbol(
                    arena, selected.scene.get(),
                    Scene::Language::Lifecycle::Update, unit));
    output << ", "_view;
    write_callback(
        output, lifecycle_symbol(
                    arena, selected.scene.get(),
                    Scene::Language::Lifecycle::Release, unit));
    auto scene_index = find_scene(scenes.get_view(), selected.scene.get());
    BAIL_IF(!scene_index);
    output << ", "_view << graphics[*scene_index].get_hosted().get_size()
           << ", &"_view
           << graphics_children_symbol(arena, selected.scene.get(), unit)
           << "},\n"_view;
  }
  output
      << "};\n\n"_view
      << "static const Tetrodotoxin::Runtime::Application::PlacementProvider "
         "application_placements[] = {\n"_view;
  for (Count index = 0; index < graphics_types.get_size(); index++) {
    output << "  "_view;
    if (graphics_placements[index].is_empty()) {
      output << "nullptr"_view;
    } else {
      output << "&"_view << graphics_placements[index];
    }
    output << ",\n"_view;
  }
  output << "};\n\n"_view
         << "static const Tetrodotoxin::Runtime::Application::ChildrenProvider "
            "application_children[] = {\n"_view;
  for (Count index = 0; index < graphics_types.get_size(); index++) {
    output << "  "_view;
    if (graphics_children[index].is_empty()) {
      output << "nullptr"_view;
    } else {
      output << "&"_view << graphics_children[index];
    }
    output << ",\n"_view;
  }
  output << "};\n\n"_view
         << "static const Tetrodotoxin::Runtime::Application::DrawableProvider "
            "application_drawables[] = {\n"_view;
  for (Count index = 0; index < graphics_types.get_size(); index++) {
    output << "  "_view;
    if (graphics_drawables[index].is_empty()) {
      output << "nullptr"_view;
    } else {
      output << "&"_view << graphics_drawables[index];
    }
    output << ",\n"_view;
  }
  output << "};\n\n"_view
         << "static const Tetrodotoxin::Runtime::Application::Transition "
            "application_transitions[] = {\n"_view;
  for (const Tetrodotoxin::Source::Reference<App::Language::Transition>& retained :
       policy->get_transitions()) {
    const App::Language::Transition& transition = retained.get();
    auto source_scene = transition.get_source_scene();
    auto signal = transition.get_signal();
    BAIL_IF(!source_scene || !signal);
    auto source_index = find_scene(scenes.get_view(), *source_scene);
    BAIL_IF(!source_index);
    Terminal::Abi::Unit unit(package, scenes[*source_index].route, artifact);
    Terminal::Abi::Symbol token(
        arena, *signal, Terminal::Abi::Symbol::Kind::ReadOnly, unit);
    Core::Option<Count> destination_index;
    auto destination = transition.get_destination_scene();
    if (destination) {
      auto selected_destination = find_scene(scenes.get_view(), *destination);
      BAIL_IF(!selected_destination);
      destination_index = *selected_destination;
    }
    output << "  {"_view << *source_index << ", &"_view << token.get_view()
           << ", Tetrodotoxin::Runtime::Application::Action::"_view;
    switch (transition.get_action()) {
    case App::Language::Transition::Action::Replace:
      output << "Replace"_view;
      break;
    case App::Language::Transition::Action::Push:
      output << "Push"_view;
      break;
    case App::Language::Transition::Action::Pop:
      output << "Pop"_view;
      break;
    case App::Language::Transition::Action::Exit:
      output << "Exit"_view;
      break;
    }
    output << ", "_view;
    if (destination_index) {
      output << *destination_index;
    } else {
      output << "Count(-1)"_view;
    }
    output << "},\n"_view;
  }
  auto initial_index = find_scene(scenes.get_view(), *initial);
  BAIL_IF(!initial_index);
  U32 width = windowed->get_width().visit(
      []() { return U32(800); }, [](U32 v) { return v; });
  U32 height = windowed->get_height().visit(
      []() { return U32(600); }, [](U32 v) { return v; });
  output << "};\n\n"_view
         << "static const Tetrodotoxin::Runtime::Application::Product "
            "application_product = {application_title, "_view
         << width << ", "_view << height << ", application_scenes, "_view
         << scenes.get_size() << ", "_view << *initial_index
         << ", application_transitions, "_view
         << policy->get_transitions().get_size()
         << ", application_placements, application_children, "
            "application_drawables, "_view
         << graphics_types.get_size() << ", application_programs, "_view
         << vulkan.get_size() << "};\n\n"_view
         << "int main() {\n  return "
            "tetrodotoxin_application_scene(&application_product);\n}\n"_view;
  return source;
}
