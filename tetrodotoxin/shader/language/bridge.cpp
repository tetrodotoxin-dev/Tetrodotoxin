// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/language/bridge.hpp"

#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

auto Shader::Language::Bridge::create(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Library::Language::TypeReference cpu,
    Library::Language::TypeReference gpu,
    Direction direction,
    Marshaling marshaling,
    Synchronization synchronization) -> Bridge& {
  return domain.construct_from<Bridge>([&]() {
    return Bridge(definition, cpu, gpu, direction, marshaling, synchronization);
  });
}

auto Shader::Language::Bridge::link(Cursor& cursor, const Abstract& context)
    -> Bool {
  auto selected_cpu = cpu.resolve_authored(cursor, context);
  auto selected_gpu = gpu.resolve_authored(cursor, context);
  BAIL_IF(!selected_cpu || !selected_gpu);

  auto library_type = selected_cpu->select<Library::Language::Model::Type>();
  auto shader_type = selected_gpu->select<Library::Language::Model::Type>();
  if (!library_type || !shader_type) {
    cursor.create_expression_error(
        definition.get_authored().get_anchor(),
        "Shader Bridge endpoints require one Library CPU Type and one Shader GPU Type."_view);
    return False;
  }
  if (&*library_type == &*shader_type) {
    cursor.create_expression_error(
        definition.get_authored().get_anchor(),
        "Shader Bridge endpoints must retain distinct CPU and GPU identities."_view);
    return False;
  }
  if (marshaling == Marshaling::Identity &&
      (!library_type->get_layout().fits(shader_type->get_layout()) ||
       !shader_type->get_layout().fits(library_type->get_layout()))) {
    cursor.create_expression_error(
        definition.get_authored().get_anchor(),
        "Identity marshaling requires matching CPU and GPU Layouts."_view,
        "Choose copy or pack marshaling when the semantic shapes differ."_view);
    return False;
  }

  cpu_type = Reference<const Tetrodotoxin::Source::Type>(*library_type);
  gpu_type = Reference<const Tetrodotoxin::Source::Type>(*shader_type);
  return True;
}

auto Shader::Language::Bridge::link_restored(const Abstract& context) -> Bool {
  Option<const Abstract&> selected_cpu;
  Option<const Abstract&> selected_gpu;
  cpu.resolve(context).visit(
      [&](const Abstract& selected) { selected_cpu = selected; },
      [](const Library::Language::TypeReference::Failure&) {});
  gpu.resolve(context).visit(
      [&](const Abstract& selected) { selected_gpu = selected; },
      [](const Library::Language::TypeReference::Failure&) {});
  BAIL_IF(!selected_cpu || !selected_gpu);

  auto library_type = selected_cpu->select<Library::Language::Model::Type>();
  auto shader_type = selected_gpu->select<Library::Language::Model::Type>();
  BAIL_IF(
      !library_type || !shader_type || &*library_type == &*shader_type ||
      (marshaling == Marshaling::Identity &&
       (!library_type->get_layout().fits(shader_type->get_layout()) ||
        !shader_type->get_layout().fits(library_type->get_layout()))));
  cpu_type = Reference<const Tetrodotoxin::Source::Type>(*library_type);
  gpu_type = Reference<const Tetrodotoxin::Source::Type>(*shader_type);
  return True;
}

auto Shader::Language::Bridge::resolve() const -> const Abstract& {
  return cpu_type && gpu_type
             ? static_cast<const Abstract&>(*this)
             : static_cast<const Abstract&>(Unknown::get_unknown());
}
