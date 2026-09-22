// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/perimortem.hpp"

#include "perimortem/graphics/frame/resource.hpp"
#include "perimortem/vulkan/context.hpp"

namespace Perimortem::Vulkan {

// TextureImage owns one device-local realization of shared Image content.
// Sampling policy does not participate in this identity, so several bindings
// can reuse the upload.
class TextureImage {
 public:
  static auto create(
      const Context& context,
      const Graphics::Frame::Resource& resource) -> TextureImage;

  TextureImage() = default;
  ~TextureImage();
  TextureImage(TextureImage&&) noexcept;
  auto operator=(TextureImage&&) noexcept -> TextureImage&;
  TextureImage(const TextureImage&) = delete;
  auto operator=(const TextureImage&) = delete;

  constexpr auto get_view() const -> VkImageView { return image_view; }

 private:
  auto release() -> void;

  VkDevice device = VK_NULL_HANDLE;
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkImageView image_view = VK_NULL_HANDLE;
};

// Texture owns one sampled descriptor over an existing TextureImage. The
// Sampler2D value selects addressing and filtering without duplicating pixels.
class Texture {
 public:
  static auto create(
      const Context& context,
      const Graphics::Frame::Resource& resource,
      VkImageView image_view,
      VkDescriptorSetLayout descriptor_set_layout) -> Texture;

  Texture() = default;
  ~Texture();
  Texture(Texture&&) noexcept;
  auto operator=(Texture&&) noexcept -> Texture&;
  Texture(const Texture&) = delete;
  auto operator=(const Texture&) = delete;

  constexpr auto get_descriptor_set() const -> VkDescriptorSet {
    return descriptor_set;
  }

 private:
  auto release() -> void;

  VkDevice device = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
  VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
};

}  // namespace Perimortem::Vulkan
