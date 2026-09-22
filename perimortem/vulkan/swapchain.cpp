// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/vulkan/swapchain.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

using namespace Perimortem::Core;
using namespace Perimortem;

static auto require_success(VkResult result, View::Bytes message) -> void {
  if (result != VK_SUCCESS) {
    Diagnostics::Log::fatal(message);
  }
}

static auto require_enumeration_read(VkResult result, View::Bytes message)
    -> void {
  if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
    Diagnostics::Log::fatal(message);
  }
}

static auto choose_surface_format(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface) -> VkSurfaceFormatKHR {
  U32 surface_format_count = 0;
  require_success(
      vkGetPhysicalDeviceSurfaceFormatsKHR(
          physical_device, surface, &surface_format_count, nullptr),
      "Vulkan: Failed to query swapchain surface formats."_view);
  if (surface_format_count == 0) {
    Diagnostics::Log::fatal("Vulkan: No swapchain surface formats found."_view);
  }

  Perimortem::Memory::Dynamic::Vector<VkSurfaceFormatKHR> surface_formats;
  surface_formats.forgetful_resize(surface_format_count);
  U32 read_surface_format_count = surface_format_count;
  require_enumeration_read(
      vkGetPhysicalDeviceSurfaceFormatsKHR(
          physical_device, surface, &read_surface_format_count,
          surface_formats.get_data()),
      "Vulkan: Failed to read swapchain surface formats."_view);
  if (read_surface_format_count == 0) {
    Diagnostics::Log::fatal("Vulkan: No swapchain surface formats found."_view);
  }

  const VkSurfaceFormatKHR preferred_surface_format = {
    VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
  if (read_surface_format_count == 1 &&
      surface_formats[0].format == VK_FORMAT_UNDEFINED) {
    return preferred_surface_format;
  }

  for (U32 i = 0; i < read_surface_format_count; i++) {
    if (surface_formats[i].format == preferred_surface_format.format &&
        surface_formats[i].colorSpace == preferred_surface_format.colorSpace) {
      return surface_formats[i];
    }
  }

  return surface_formats[0];
}

static auto choose_present_mode(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface) -> VkPresentModeKHR {
  U32 present_mode_count = 0;
  require_success(
      vkGetPhysicalDeviceSurfacePresentModesKHR(
          physical_device, surface, &present_mode_count, nullptr),
      "Vulkan: Failed to query swapchain present modes."_view);
  if (present_mode_count == 0) {
    Diagnostics::Log::fatal("Vulkan: No swapchain present modes found."_view);
  }

  Perimortem::Memory::Dynamic::Vector<VkPresentModeKHR> present_modes;
  present_modes.forgetful_resize(present_mode_count);
  U32 read_present_mode_count = present_mode_count;
  require_enumeration_read(
      vkGetPhysicalDeviceSurfacePresentModesKHR(
          physical_device, surface, &read_present_mode_count,
          present_modes.get_data()),
      "Vulkan: Failed to read swapchain present modes."_view);
  if (read_present_mode_count == 0) {
    Diagnostics::Log::fatal("Vulkan: No swapchain present modes found."_view);
  }

  for (U32 i = 0; i < read_present_mode_count; i++) {
    if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      return VK_PRESENT_MODE_MAILBOX_KHR;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

auto Vulkan::Swapchain::build(
    const Vulkan::Context& ctx,
    U32 width,
    U32 height,
    VkSwapchainKHR old_swapchain) -> Vulkan::Swapchain {
  Vulkan::Swapchain swapchain_state;
  swapchain_state.device = ctx.get_device();
  const VkSurfaceKHR surface = ctx.get_surface();

  const VkSurfaceFormatKHR surface_format =
      choose_surface_format(ctx.get_physical_device(), surface);
  const VkPresentModeKHR present_mode =
      choose_present_mode(ctx.get_physical_device(), surface);

  swapchain_state.format = surface_format.format;
  swapchain_state.extent = {width, height};

  VkSurfaceCapabilitiesKHR surface_capabilities = {};
  require_success(
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
          ctx.get_physical_device(), surface, &surface_capabilities),
      "Vulkan: Failed to query swapchain surface capabilities."_view);

  // Request one more than the minimum to avoid stalling on the driver.
  U32 image_count = surface_capabilities.minImageCount + 1;
  if (surface_capabilities.maxImageCount > 0 &&
      image_count > surface_capabilities.maxImageCount) {
    image_count = surface_capabilities.maxImageCount;
  }

  const U32 queue_family = ctx.get_graphics_queue_family();

  VkSwapchainCreateInfoKHR swapchain_info = {
    VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  swapchain_info.surface = surface;
  swapchain_info.minImageCount = image_count;
  swapchain_info.imageFormat = swapchain_state.format;
  swapchain_info.imageColorSpace = surface_format.colorSpace;
  swapchain_info.imageExtent = swapchain_state.extent;
  swapchain_info.imageArrayLayers = 1;
  swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchain_info.queueFamilyIndexCount = 1;
  swapchain_info.pQueueFamilyIndices = &queue_family;
  swapchain_info.preTransform = surface_capabilities.currentTransform;
  swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchain_info.presentMode = present_mode;
  swapchain_info.clipped = VK_TRUE;
  swapchain_info.oldSwapchain = old_swapchain;

  require_success(
      vkCreateSwapchainKHR(
          swapchain_state.device, &swapchain_info, nullptr,
          &swapchain_state.swapchain),
      "Vulkan: Failed to create swapchain."_view);

  swapchain_state.image_count = 0;
  require_success(
      vkGetSwapchainImagesKHR(
          swapchain_state.device, swapchain_state.swapchain,
          &swapchain_state.image_count, nullptr),
      "Vulkan: Failed to query swapchain images."_view);
  if (swapchain_state.image_count > max_images) {
    Diagnostics::Log::fatal("Vulkan: Too many swapchain images."_view);
  }

  require_enumeration_read(
      vkGetSwapchainImagesKHR(
          swapchain_state.device, swapchain_state.swapchain,
          &swapchain_state.image_count, swapchain_state.images.get_data()),
      "Vulkan: Failed to read swapchain images."_view);
  if (swapchain_state.image_count == 0) {
    Diagnostics::Log::fatal("Vulkan: No swapchain images found."_view);
  }

  for (U32 i = 0; i < swapchain_state.image_count; i++) {
    VkImageViewCreateInfo image_view_info = {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    image_view_info.image = swapchain_state.images[i];
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = swapchain_state.format;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    require_success(
        vkCreateImageView(
            swapchain_state.device, &image_view_info, nullptr,
            &swapchain_state.image_views[i]),
        "Vulkan: Failed to create swapchain image view."_view);
  }

  return swapchain_state;
}

auto Vulkan::Swapchain::create(
    const Vulkan::Context& ctx,
    U32 width,
    U32 height) -> Vulkan::Swapchain {
  return build(ctx, width, height, VK_NULL_HANDLE);
}

auto Vulkan::Swapchain::recreate(
    const Vulkan::Context& ctx,
    U32 width,
    U32 height) -> void {
  const VkSwapchainKHR old_swapchain = swapchain;

  // Destroy image views but leave the old swapchain alive for oldSwapchain.
  for (U32 i = 0; i < image_count; i++) {
    vkDestroyImageView(device, image_views[i], nullptr);
    image_views[i] = VK_NULL_HANDLE;
  }

  image_count = 0;
  swapchain = VK_NULL_HANDLE;

  *this = build(ctx, width, height, old_swapchain);

  vkDestroySwapchainKHR(device, old_swapchain, nullptr);
}

auto Vulkan::Swapchain::destroy(VkDevice target_device) -> void {
  for (U32 i = 0; i < image_count; i++) {
    vkDestroyImageView(target_device, image_views[i], nullptr);
  }

  vkDestroySwapchainKHR(target_device, swapchain, nullptr);
}

Vulkan::Swapchain::~Swapchain() {
  if (device && swapchain) {
    destroy(device);
  }
}

Vulkan::Swapchain::Swapchain(Vulkan::Swapchain&& other) noexcept
    : device(other.device),
      swapchain(other.swapchain),
      format(other.format),
      extent(other.extent),
      images(other.images),
      image_views(other.image_views),
      image_count(other.image_count) {
  other.swapchain = VK_NULL_HANDLE;
  other.image_count = 0;
}

auto Vulkan::Swapchain::operator=(Vulkan::Swapchain&& other) noexcept
    -> Vulkan::Swapchain& {
  if (this != &other) {
    if (device && swapchain) {
      destroy(device);
    }

    device = other.device;
    swapchain = other.swapchain;
    format = other.format;
    extent = other.extent;
    images = other.images;
    image_views = other.image_views;
    image_count = other.image_count;
    other.swapchain = VK_NULL_HANDLE;
    other.image_count = 0;
  }

  return *this;
}

auto Vulkan::Swapchain::acquire_next_image(VkSemaphore signal_semaphore) const
    -> U32 {
  U32 index = 0;
  const VkResult result = vkAcquireNextImageKHR(
      device, swapchain, UINT64_MAX, signal_semaphore, VK_NULL_HANDLE, &index);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    return UINT32_MAX;
  }

  return index;
}

auto Vulkan::Swapchain::get_swapchain() const -> VkSwapchainKHR {
  return swapchain;
}

auto Vulkan::Swapchain::get_extent() const -> VkExtent2D {
  return extent;
}

auto Vulkan::Swapchain::get_format() const -> VkFormat {
  return format;
}

auto Vulkan::Swapchain::get_image(U32 index) const -> VkImage {
  return images[index];
}

auto Vulkan::Swapchain::get_image_view(U32 index) const -> VkImageView {
  return image_views[index];
}

auto Vulkan::Swapchain::get_image_count() const -> U32 {
  return image_count;
}
