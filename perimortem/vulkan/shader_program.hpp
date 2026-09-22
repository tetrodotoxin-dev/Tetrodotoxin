// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"

#include "perimortem/graphics/frame/pipeline.hpp"
#include "perimortem/vulkan/description/program.hpp"

namespace Perimortem::Vulkan {

// Owns a Vulkan graphics pipeline built from one Description::Program and one
// backend-neutral fixed-state selection. Per-draw values such as vertex count
// and host inputs are supplied to command methods and are never retained.
class ShaderProgram {
 public:
  static auto create(
      VkDevice device,
      VkFormat color_format,
      const Description::Program& source,
      Perimortem::Graphics::Frame::Pipeline pipeline,
      Core::View::Vector<VkDescriptorSetLayout> descriptor_set_layouts)
      -> ShaderProgram;

  ShaderProgram() = default;
  ~ShaderProgram();
  ShaderProgram(ShaderProgram&&) noexcept;
  auto operator=(ShaderProgram&&) noexcept -> ShaderProgram&;
  ShaderProgram(const ShaderProgram&) = delete;
  auto operator=(const ShaderProgram&) = delete;

  auto bind(VkCommandBuffer command_buffer) const -> void;
  auto bind_descriptor_set(
      VkCommandBuffer command_buffer,
      VkDescriptorSet descriptor_set,
      Count set = 0) const -> void;
  auto push_constants(
      VkCommandBuffer command_buffer,
      Core::View::Bytes source,
      Count range_index = 0) const -> void;
  auto draw(VkCommandBuffer command_buffer, Count vertex_count) const -> void;

  auto get_layout() const -> VkPipelineLayout;
  auto get_pipeline() const -> VkPipeline;

 private:
  static constexpr Count max_push_constant_ranges = 8;

  VkDevice device = VK_NULL_HANDLE;
  VkPipelineLayout layout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;
  Core::Static::Vector<VkPushConstantRange, max_push_constant_ranges>
      push_constant_ranges;
  Count push_constant_count = 0;
  Count descriptor_set_count = 0;
};

}  // namespace Perimortem::Vulkan
