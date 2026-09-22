// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/vulkan/description/descriptor_binding.hpp"
#include "perimortem/vulkan/description/host_field.hpp"
#include "perimortem/vulkan/description/host_input_range.hpp"
#include "perimortem/vulkan/description/module.hpp"
#include "perimortem/vulkan/description/vertex_input.hpp"

namespace Perimortem::Vulkan::Description {

// A borrowed description of one Vulkan program derived from completed Pipeline,
// Shader, and SPIR V products. Program groups target modules with their host
// input and resource layouts, but owns none of the referenced arrays or names.
// A frame draw supplies fixed pipeline state when Vulkan realizes these stages.
//
// Per draw values such as fixed state, vertex count, and host input bytes do
// not live here. Keeping those transactions separate prevents this target
// description from becoming another frame model.
struct Program {
  const U8* locator = nullptr;
  Core::View::Vector<Module> modules;
  Core::View::Vector<HostInputRange> host_input_ranges;
  Core::View::Vector<DescriptorBinding> descriptors;
  Core::View::Vector<HostField> host_fields;
  Core::View::Vector<VertexInput> vertex_inputs;
  Count host_size = 0;
  Count parameters_offset = 0;
  Count parameters_size = 0;
  Bool requires_float64 = False;
};

}  // namespace Perimortem::Vulkan::Description
