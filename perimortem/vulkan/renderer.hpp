// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/static/vector.hpp"

#include "perimortem/vulkan/context.hpp"
#include "perimortem/vulkan/swapchain.hpp"

namespace Perimortem::Vulkan {

// Vulkan device, swapchain, and frame mechanics for a platform window. The
// backend deliberately has no knowledge of scenes, sprites, or event policy.
class Renderer {
 public:
  // One acquired frame. Graphics records its draw commands between
  // begin_frame() and end_frame().
  struct Frame {
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    U32 width = 0;
    U32 height = 0;
    Count frame_index = 0;
    U32 image_index = 0;
  };

  Renderer(System::Presentation presentation, U32 width, U32 height);
  ~Renderer();
  Renderer(const Renderer&) = delete;
  auto operator=(const Renderer&) -> Renderer& = delete;

  // Rebuilds the swapchain for a new physical extent and reports whether its
  // color format changed, which requires Graphics pipelines to be rebuilt.
  auto resize(U32 width, U32 height) -> Bool;
  auto begin_frame(Frame& frame) -> Bool;
  auto end_frame(const Frame& frame) -> void;
  auto wait_idle() -> void;

  auto get_width() const -> U32;
  auto get_height() const -> U32;
  auto get_context() const -> const Context&;
  auto get_swapchain() const -> const Swapchain&;

 private:
  static constexpr Count frames_in_flight = 2;

  struct FrameState {
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkSemaphore image_available = VK_NULL_HANDLE;
    VkSemaphore render_finished = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
  };

  static auto require_success(VkResult result, Core::View::Bytes message)
      -> void;
  auto allocate_frames() -> void;
  auto destroy_frames() -> void;
  auto refresh_swapchain_images() -> void;
  auto begin_rendering(const Frame& frame) -> void;
  auto finish_rendering(const Frame& frame) -> void;

  Context context;
  Swapchain swapchain;
  Core::Static::Vector<FrameState, frames_in_flight> frames;
  Core::Static::Vector<VkImage, Swapchain::max_images> swapchain_images;
  Count current_frame = 0;
};

}  // namespace Perimortem::Vulkan
