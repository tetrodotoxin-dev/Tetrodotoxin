// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/vulkan/context.hpp"

namespace Perimortem::Vulkan {

// Owns the swapchain, its images, and image views. Supports recreation
// in place when the window is resized or the buffer scale changes.
class Swapchain {
 public:
  static auto create(const Context& ctx, U32 width, U32 height) -> Swapchain;

  Swapchain() = default;
  ~Swapchain();
  Swapchain(Swapchain&&) noexcept;
  auto operator=(Swapchain&&) noexcept -> Swapchain&;
  Swapchain(const Swapchain&) = delete;
  auto operator=(const Swapchain&) = delete;

  // Destroys and recreates the swapchain at the new physical pixel dimensions.
  auto recreate(const Context& ctx, U32 width, U32 height) -> void;

  // Acquires the next presentable image index. Blocks until one is available.
  // Returns UINT32_MAX on a resize signal (caller should recreate).
  auto acquire_next_image(VkSemaphore signal_semaphore) const -> U32;

  auto get_swapchain() const -> VkSwapchainKHR;
  auto get_extent() const -> VkExtent2D;
  auto get_format() const -> VkFormat;
  auto get_image(U32 index) const -> VkImage;
  auto get_image_view(U32 index) const -> VkImageView;
  auto get_image_count() const -> U32;

  static constexpr U32 max_images = 8;

 private:
  auto destroy(VkDevice target_device) -> void;
  static auto build(
      const Context& ctx,
      U32 width,
      U32 height,
      VkSwapchainKHR old_swapchain) -> Swapchain;

  VkDevice device = VK_NULL_HANDLE;
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  VkFormat format = VK_FORMAT_UNDEFINED;
  VkExtent2D extent = {};

  Core::Static::Vector<VkImage, max_images> images;
  Core::Static::Vector<VkImageView, max_images> image_views;
  U32 image_count = 0;
};

}  // namespace Perimortem::Vulkan
