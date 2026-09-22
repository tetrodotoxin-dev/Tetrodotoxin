// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/vulkan/texture.hpp"

#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/graphics/image.hpp"
#include "perimortem/graphics/pixel.hpp"
#include "perimortem/graphics/sampler_2d.hpp"

using namespace Perimortem;

static auto require_success(
    VkResult result,
    Perimortem::Core::View::Bytes message) -> void {
  if (result != VK_SUCCESS) {
    Perimortem::Core::Diagnostics::Log::fatal(message);
  }
}

static auto allocate_memory(
    const Vulkan::Context& context,
    VkMemoryRequirements requirements,
    VkMemoryPropertyFlags properties) -> VkDeviceMemory {
  VkMemoryAllocateInfo info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  info.allocationSize = requirements.size;
  info.memoryTypeIndex =
      context.find_memory_type(requirements.memoryTypeBits, properties);
  if (info.memoryTypeIndex == UINT32_MAX) {
    Perimortem::Core::Diagnostics::Log::fatal(
        "Vulkan: No compatible texture memory type found."_view);
  }

  VkDeviceMemory memory = VK_NULL_HANDLE;
  require_success(
      vkAllocateMemory(context.get_device(), &info, nullptr, &memory),
      "Vulkan: Failed to allocate texture memory."_view);
  return memory;
}

static auto transition_image_layout(
    VkCommandBuffer command_buffer,
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkPipelineStageFlags2 source_stage,
    VkAccessFlags2 source_access,
    VkPipelineStageFlags2 destination_stage,
    VkAccessFlags2 destination_access) -> void {
  VkImageMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
  barrier.srcStageMask = source_stage;
  barrier.srcAccessMask = source_access;
  barrier.dstStageMask = destination_stage;
  barrier.dstAccessMask = destination_access;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkDependencyInfo dependency = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
  dependency.imageMemoryBarrierCount = 1;
  dependency.pImageMemoryBarriers = &barrier;
  vkCmdPipelineBarrier2(command_buffer, &dependency);
}

auto Vulkan::TextureImage::create(
    const Vulkan::Context& context,
    const Graphics::Frame::Resource& resource) -> TextureImage {
  TextureImage texture;
  texture.device = context.get_device();

  auto source_image = Graphics::Image::retain(resource.get_object());
  BAIL_IF(!source_image);
  Core::View::Vector<Graphics::Pixel> pixels = source_image->get_pixels();
  const U32 width = source_image->get_width();
  const U32 height = source_image->get_height();
  const VkDeviceSize image_size =
      VkDeviceSize(width) * height * Graphics::Pixel::get_byte_count();
  if (width == 0 || height == 0 || resource.is_empty() ||
      pixels.get_size() * sizeof(Graphics::Pixel) < image_size) {
    return texture;
  }

  VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = image_size;
  buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBuffer staging = VK_NULL_HANDLE;
  require_success(
      vkCreateBuffer(context.get_device(), &buffer_info, nullptr, &staging),
      "Vulkan: Failed to create texture staging buffer."_view);

  VkMemoryRequirements staging_requirements = {};
  vkGetBufferMemoryRequirements(
      context.get_device(), staging, &staging_requirements);
  constexpr VkMemoryPropertyFlags host_flags =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  VkDeviceMemory staging_memory =
      allocate_memory(context, staging_requirements, host_flags);
  require_success(
      vkBindBufferMemory(context.get_device(), staging, staging_memory, 0),
      "Vulkan: Failed to bind texture staging memory."_view);

  void* mapped = nullptr;
  require_success(
      vkMapMemory(
          context.get_device(), staging_memory, 0, image_size, 0, &mapped),
      "Vulkan: Failed to map texture staging memory."_view);
  Core::View::Bytes source_bytes(
      Core::Data::cast<const U8>(pixels.get_data()), Count(image_size));
  Core::Access::Bytes destination_bytes(
      Core::Data::cast<U8>(mapped), Count(image_size));
  for (Count index = 0; index < source_bytes.get_size(); index++) {
    destination_bytes.get_data()[index] = source_bytes[index];
  }
  vkUnmapMemory(context.get_device(), staging_memory);

  VkImageCreateInfo image_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
  image_info.extent = {width, height, 1};
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  require_success(
      vkCreateImage(
          context.get_device(), &image_info, nullptr, &texture.image),
      "Vulkan: Failed to create texture image."_view);

  VkMemoryRequirements image_requirements = {};
  vkGetImageMemoryRequirements(
      context.get_device(), texture.image, &image_requirements);
  texture.memory = allocate_memory(
      context, image_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  require_success(
      vkBindImageMemory(
          context.get_device(), texture.image, texture.memory, 0),
      "Vulkan: Failed to bind texture image memory."_view);

  VkCommandBuffer command_buffer = context.begin_immediate_commands();
  transition_image_layout(
      command_buffer, texture.image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VkAccessFlags2(0),
      VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
  VkBufferImageCopy region = {};
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.layerCount = 1;
  region.imageExtent = {width, height, 1};
  vkCmdCopyBufferToImage(
      command_buffer, staging, texture.image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  transition_image_layout(
      command_buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
      VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
  context.submit_immediate_commands(command_buffer);

  vkDestroyBuffer(context.get_device(), staging, nullptr);
  vkFreeMemory(context.get_device(), staging_memory, nullptr);

  VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  view_info.image = texture.image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.layerCount = 1;
  require_success(
      vkCreateImageView(
          context.get_device(), &view_info, nullptr, &texture.image_view),
      "Vulkan: Failed to create texture image view."_view);
  return texture;
}

auto Vulkan::TextureImage::release() -> void {
  if (!device) {
    return;
  }

  vkDestroyImageView(device, image_view, nullptr);
  vkDestroyImage(device, image, nullptr);
  vkFreeMemory(device, memory, nullptr);
  device = VK_NULL_HANDLE;
  image = VK_NULL_HANDLE;
  memory = VK_NULL_HANDLE;
  image_view = VK_NULL_HANDLE;
}

Vulkan::TextureImage::~TextureImage() {
  release();
}

Vulkan::TextureImage::TextureImage(TextureImage&& other) noexcept
    : device(other.device),
      image(other.image),
      memory(other.memory),
      image_view(other.image_view) {
  other.device = VK_NULL_HANDLE;
}

auto Vulkan::TextureImage::operator=(TextureImage&& other) noexcept
    -> TextureImage& {
  if (this == &other) {
    return *this;
  }

  release();
  device = other.device;
  image = other.image;
  memory = other.memory;
  image_view = other.image_view;
  other.device = VK_NULL_HANDLE;
  return *this;
}

auto Vulkan::Texture::create(
    const Vulkan::Context& context,
    const Graphics::Frame::Resource& resource,
    VkImageView image_view,
    VkDescriptorSetLayout descriptor_set_layout) -> Texture {
  Texture texture;
  texture.device = context.get_device();
  BAIL_IF(image_view == VK_NULL_HANDLE || resource.is_empty());

  VkSamplerCreateInfo sampler_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  VkFilter filter = VK_FILTER_LINEAR;
  switch (resource.get_sampler().get_filtering()) {
  case Graphics::Sampler2D::Filtering::Linear:
    filter = VK_FILTER_LINEAR;
    break;
  case Graphics::Sampler2D::Filtering::Nearest:
    filter = VK_FILTER_NEAREST;
    break;
  }
  sampler_info.magFilter = filter;
  sampler_info.minFilter = filter;

  VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  switch (resource.get_sampler().get_addressing()) {
  case Graphics::Sampler2D::Addressing::Zero:
    address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    break;
  case Graphics::Sampler2D::Addressing::Clamp:
    address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    break;
  case Graphics::Sampler2D::Addressing::Wrap:
    address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    break;
  }
  sampler_info.addressModeU = address_mode;
  sampler_info.addressModeV = address_mode;
  sampler_info.addressModeW = address_mode;
  require_success(
      vkCreateSampler(
          context.get_device(), &sampler_info, nullptr, &texture.sampler),
      "Vulkan: Failed to create texture sampler."_view);

  VkDescriptorPoolSize pool_size = {};
  pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_size.descriptorCount = 1;
  VkDescriptorPoolCreateInfo pool_info = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  pool_info.maxSets = 1;
  pool_info.poolSizeCount = 1;
  pool_info.pPoolSizes = &pool_size;
  require_success(
      vkCreateDescriptorPool(
          context.get_device(), &pool_info, nullptr, &texture.descriptor_pool),
      "Vulkan: Failed to create texture descriptor pool."_view);

  VkDescriptorSetAllocateInfo set_info = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  set_info.descriptorPool = texture.descriptor_pool;
  set_info.descriptorSetCount = 1;
  set_info.pSetLayouts = &descriptor_set_layout;
  require_success(
      vkAllocateDescriptorSets(
          context.get_device(), &set_info, &texture.descriptor_set),
      "Vulkan: Failed to allocate texture descriptor set."_view);

  VkDescriptorImageInfo descriptor = {};
  descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  descriptor.imageView = image_view;
  descriptor.sampler = texture.sampler;
  VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  write.dstSet = texture.descriptor_set;
  write.dstBinding = 0;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.pImageInfo = &descriptor;
  vkUpdateDescriptorSets(context.get_device(), 1, &write, 0, nullptr);
  return texture;
}

auto Vulkan::Texture::release() -> void {
  if (!device) {
    return;
  }

  vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
  vkDestroySampler(device, sampler, nullptr);
  device = VK_NULL_HANDLE;
  sampler = VK_NULL_HANDLE;
  descriptor_pool = VK_NULL_HANDLE;
  descriptor_set = VK_NULL_HANDLE;
}

Vulkan::Texture::~Texture() {
  release();
}

Vulkan::Texture::Texture(Texture&& other) noexcept
    : device(other.device),
      sampler(other.sampler),
      descriptor_pool(other.descriptor_pool),
      descriptor_set(other.descriptor_set) {
  other.device = VK_NULL_HANDLE;
}

auto Vulkan::Texture::operator=(Texture&& other) noexcept -> Texture& {
  if (this == &other) {
    return *this;
  }

  release();
  device = other.device;
  sampler = other.sampler;
  descriptor_pool = other.descriptor_pool;
  descriptor_set = other.descriptor_set;
  other.device = VK_NULL_HANDLE;
  return *this;
}
