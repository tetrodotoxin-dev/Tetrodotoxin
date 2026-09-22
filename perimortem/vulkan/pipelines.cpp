// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/vulkan/pipelines.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/graphics/image.hpp"

using namespace Perimortem::Core;
using namespace Perimortem;

struct UnitQuadVertex {
  R32 position[2];
  R32 texture_uv[2];
};

static auto supports_unit_quad(const Vulkan::Description::Program& description)
    -> Bool {
  auto inputs = description.vertex_inputs;
  return Bool(
      inputs.get_size() == 2 && inputs.get_data()[0].location == 0 &&
      inputs.get_data()[0].components == 2 &&
      inputs.get_data()[0].offset == 0 &&
      inputs.get_data()[0].stride == sizeof(UnitQuadVertex) &&
      inputs.get_data()[1].location == 1 &&
      inputs.get_data()[1].components == 2 &&
      inputs.get_data()[1].offset == sizeof(R32) * 2 &&
      inputs.get_data()[1].stride == sizeof(UnitQuadVertex));
}

const Core::Object<>::Descriptor Vulkan::Pipelines::cache_descriptor(
    sizeof(TextureCacheEntry),
    alignof(TextureCacheEntry),
    Pipelines::finalize_cache);

const Core::Object<>::Descriptor Vulkan::Pipelines::image_cache_descriptor(
    sizeof(ImageCacheEntry),
    alignof(ImageCacheEntry),
    Pipelines::finalize_image_cache);

const Core::Object<>::Descriptor Vulkan::Pipelines::realization_descriptor(
    sizeof(Realization),
    alignof(Realization),
    Pipelines::finalize_realization);

Vulkan::Pipelines::Pipelines(
    const Context& context,
    VkFormat color_format,
    View::Vector<Description::Program> descriptions)
    : context(context), descriptions(descriptions) {
  validate_descriptions();
  create_descriptor_layout();
  create_vertex_buffer();
  rebuild(color_format);
}

Vulkan::Pipelines::~Pipelines() {
  vkDeviceWaitIdle(context.get_device());
  for (Core::Object<> texture : textures.get_view()) {
    texture.release();
  }
  textures.clear();
  for (Core::Object<> image : images.get_view()) {
    image.release();
  }
  images.clear();
  for (Core::Object<> realization : realizations.get_view()) {
    realization.release();
  }
  realizations.clear();
  destroy_vertex_buffer();
  if (descriptor_layout) {
    vkDestroyDescriptorSetLayout(
        context.get_device(), descriptor_layout, nullptr);
  }
}

auto Vulkan::Pipelines::validate_descriptions() const -> void {
  if (descriptions.is_empty()) {
    Diagnostics::Log::fatal(
        "Vulkan: No generated Programs were supplied."_view);
  }
  for (Count index = 0; index < descriptions.get_size(); index++) {
    const Description::Program& description = descriptions.get_data()[index];
    if (description.locator == nullptr || description.modules.is_empty() ||
        description.host_input_ranges.get_size() != 1 ||
        description.host_size == 0 ||
        description.parameters_offset > description.host_size ||
        description.parameters_size >
            description.host_size - description.parameters_offset ||
        description.descriptors.is_empty() ||
        (description.requires_float64 && !context.supports_float64())) {
      Diagnostics::Log::fatal(
          "Vulkan: A generated Program is not supported by this device."_view);
    }
    for (Count descriptor_index = 0;
         descriptor_index < description.descriptors.get_size();
         descriptor_index++) {
      const Description::DescriptorBinding& descriptor =
          description.descriptors[descriptor_index];
      if (descriptor.slot != 0 ||
          descriptor.resource != Description::Resource::SampledTexture2D) {
        Diagnostics::Log::fatal(
            "Vulkan: A generated descriptor is not supported by this device."_view);
      }
      for (Count prior = 0; prior < descriptor_index; prior++) {
        if (description.descriptors[prior].set == descriptor.set) {
          Diagnostics::Log::fatal(
              "Vulkan: Generated texture descriptors use distinct sets."_view);
        }
      }
    }
    for (Count previous = 0; previous < index; previous++) {
      if (descriptions.get_data()[previous].locator == description.locator) {
        Diagnostics::Log::fatal(
            "Vulkan: Generated Program locators must be unique."_view);
      }
    }
  }
}

auto Vulkan::Pipelines::create_descriptor_layout() -> void {
  VkDescriptorSetLayoutBinding binding = {};
  binding.binding = 0;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.descriptorCount = 1;
  binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo info = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  info.bindingCount = 1;
  info.pBindings = &binding;
  if (vkCreateDescriptorSetLayout(
          context.get_device(), &info, nullptr, &descriptor_layout) !=
      VK_SUCCESS) {
    Diagnostics::Log::fatal(
        "Vulkan: Failed to create the generated descriptor layout."_view);
  }
}

auto Vulkan::Pipelines::create_vertex_buffer() -> void {
  static constexpr UnitQuadVertex vertices[] = {
    {{0.0f, 0.0f}, {0.0f, 0.0f}}, {{1.0f, 0.0f}, {1.0f, 0.0f}},
    {{1.0f, 1.0f}, {1.0f, 1.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}},
    {{1.0f, 1.0f}, {1.0f, 1.0f}}, {{0.0f, 1.0f}, {0.0f, 1.0f}},
  };

  VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = sizeof(vertices);
  buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateBuffer(
          context.get_device(), &buffer_info, nullptr, &vertex_buffer) !=
      VK_SUCCESS) {
    Diagnostics::Log::fatal("Vulkan: Failed to create vertex buffer."_view);
  }

  VkMemoryRequirements requirements = {};
  vkGetBufferMemoryRequirements(
      context.get_device(), vertex_buffer, &requirements);
  constexpr VkMemoryPropertyFlags properties =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  U32 memory_type =
      context.find_memory_type(requirements.memoryTypeBits, properties);
  if (memory_type == UINT32_MAX) {
    Diagnostics::Log::fatal("Vulkan: No compatible vertex memory."_view);
  }
  VkMemoryAllocateInfo allocation = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocation.allocationSize = requirements.size;
  allocation.memoryTypeIndex = memory_type;
  if (vkAllocateMemory(
          context.get_device(), &allocation, nullptr, &vertex_memory) !=
          VK_SUCCESS ||
      vkBindBufferMemory(
          context.get_device(), vertex_buffer, vertex_memory, 0) !=
          VK_SUCCESS) {
    Diagnostics::Log::fatal("Vulkan: Failed to allocate vertex memory."_view);
  }

  void* mapped = nullptr;
  if (vkMapMemory(
          context.get_device(), vertex_memory, 0, sizeof(vertices), 0,
          &mapped) != VK_SUCCESS) {
    Diagnostics::Log::fatal("Vulkan: Failed to map vertex memory."_view);
  }
  Data::copy(
      Data::cast<U8>(mapped), Data::cast<const U8>(vertices), sizeof(vertices));
  vkUnmapMemory(context.get_device(), vertex_memory);
}

auto Vulkan::Pipelines::destroy_vertex_buffer() -> void {
  if (vertex_buffer) {
    vkDestroyBuffer(context.get_device(), vertex_buffer, nullptr);
    vertex_buffer = VK_NULL_HANDLE;
  }
  if (vertex_memory) {
    vkFreeMemory(context.get_device(), vertex_memory, nullptr);
    vertex_memory = VK_NULL_HANDLE;
  }
}

auto Vulkan::Pipelines::rebuild(VkFormat color_format) -> void {
  for (Core::Object<> realization : realizations.get_view()) {
    realization.release();
  }
  realizations.clear();
  this->color_format = color_format;
}

auto Vulkan::Pipelines::find_description(const U8* locator) const
    -> const Description::Program* {
  for (Count index = 0; index < descriptions.get_size(); index++) {
    const Description::Program& description = descriptions.get_data()[index];
    if (description.locator == locator) {
      return &description;
    }
  }

  return nullptr;
}

auto Vulkan::Pipelines::find_realization(
    const U8* locator,
    Perimortem::Graphics::Frame::Pipeline pipeline) -> Realization* {
  for (Core::Object<> object : realizations.get_view()) {
    Realization& realization = get_realization(object);
    if (
        realization.description->locator == locator &&
        realization.pipeline == pipeline) {
      return &realization;
    }
  }
  return nullptr;
}

auto Vulkan::Pipelines::find_realization(
    const U8* locator,
    Perimortem::Graphics::Frame::Pipeline pipeline) const
    -> const Realization* {
  for (Core::Object<> object : realizations.get_view()) {
    const Realization& realization = get_realization(object);
    if (
        realization.description->locator == locator &&
        realization.pipeline == pipeline) {
      return &realization;
    }
  }
  return nullptr;
}

auto Vulkan::Pipelines::realize_pipeline(
    const Perimortem::Graphics::Frame::Batch& batch) -> Realization* {
  const U8* locator = batch.get_program().get_locator();
  Perimortem::Graphics::Frame::Pipeline pipeline = batch.get_pipeline();
  Realization* retained = find_realization(locator, pipeline);
  if (retained != nullptr) {
    return retained;
  }

  const Description::Program* description = find_description(locator);
  if (description == nullptr || color_format == VK_FORMAT_UNDEFINED) {
    return nullptr;
  }

  Count set_count = 0;
  for (const Description::DescriptorBinding& descriptor :
       description->descriptors) {
    set_count = Core::Math::max(set_count, descriptor.set + 1);
  }
  Memory::Dynamic::Vector<VkDescriptorSetLayout> layouts;
  for (Count set = 0; set < set_count; set++) {
    layouts.insert(descriptor_layout);
  }

  Core::Object<> storage = Core::Object<>::create(realization_descriptor);
  new (storage.get_payload(), Core::Placement::Construct) Realization();
  Realization& realization = get_realization(storage);
  realization.description = description;
  realization.pipeline = pipeline;
  realization.shader = ShaderProgram::create(
      context.get_device(), color_format, *description, pipeline,
      layouts.get_view());
  realizations.emplace(static_cast<Core::Object<>&&>(storage));
  return &get_realization(realizations[realizations.get_size() - 1]);
}

auto Vulkan::Pipelines::validate(
    View::Vector<Perimortem::Graphics::Frame::Batch> batches) const -> Bool {
  for (const Perimortem::Graphics::Frame::Batch& batch : batches) {
    const Description::Program* description =
        find_description(batch.get_program().get_locator());
    Perimortem::Graphics::Frame::Pipeline pipeline = batch.get_pipeline();
    BAIL_IF(
        description == nullptr || batch.get_vertex_count() == 0 ||
        batch.get_vertex_count() > U32(-1) ||
        pipeline.get_geometry() !=
            Perimortem::Graphics::Frame::Pipeline::Geometry::UnitQuad2D ||
        pipeline.get_topology() !=
            Perimortem::Graphics::Frame::Pipeline::Topology::TriangleList ||
        pipeline.get_blend() !=
            Perimortem::Graphics::Frame::Pipeline::Blend::Alpha ||
        !supports_unit_quad(*description) ||
        batch.get_resources().get_size() !=
            description->descriptors.get_size() ||
        batch.get_inputs().get_size() != description->parameters_size ||
        batch.get_size_pixels().width == 0 ||
        batch.get_size_pixels().height == 0);
    for (const Perimortem::Graphics::Frame::Resource& resource :
         batch.get_resources()) {
      auto image = Perimortem::Graphics::Image::retain(resource.get_object());
      BAIL_IF(!image || !image->is_drawable());
    }
  }
  return True;
}

auto Vulkan::Pipelines::record(
    VkCommandBuffer command_buffer,
    U32 width,
    U32 height,
    View::Vector<Perimortem::Graphics::Frame::Batch> batches) -> Bool {
  BAIL_IF(!command_buffer || width == 0 || height == 0 || !validate(batches));

  // Resource realization finishes before command recording begins. A rejected
  // Texture or Program therefore leaves no partial draw sequence in this frame.
  for (const Perimortem::Graphics::Frame::Batch& batch : batches) {
    BAIL_IF(realize_pipeline(batch) == nullptr);
    for (const Perimortem::Graphics::Frame::Resource& resource :
         batch.get_resources()) {
      BAIL_IF(!realize_image(resource) || !realize_texture(resource));
    }
  }
  for (const Perimortem::Graphics::Frame::Batch& batch : batches) {
    Realization* realization = find_realization(
        batch.get_program().get_locator(), batch.get_pipeline());
    BAIL_IF(realization == nullptr);
    Memory::Dynamic::Bytes inputs =
        make_host_inputs(*realization->description, batch, width, height);
    BAIL_IF(inputs.get_size() != realization->description->host_size);
    realization->shader.bind(command_buffer);
    constexpr VkDeviceSize vertex_offset = 0;
    vkCmdBindVertexBuffers(
        command_buffer, 0, 1, &vertex_buffer, &vertex_offset);
    for (Count descriptor_index = 0;
         descriptor_index < realization->description->descriptors.get_size();
         descriptor_index++) {
      Texture* texture = find_texture(batch.get_resources()[descriptor_index]);
      BAIL_IF(texture == nullptr);
      realization->shader.bind_descriptor_set(
          command_buffer, texture->get_descriptor_set(),
          realization->description->descriptors[descriptor_index].set);
    }
    realization->shader.push_constants(command_buffer, inputs.get_view());
    realization->shader.draw(command_buffer, batch.get_vertex_count());
  }

  sweep_textures();
  return True;
}

auto Vulkan::Pipelines::make_host_inputs(
    const Description::Program& description,
    const Perimortem::Graphics::Frame::Batch& batch,
    U32 width,
    U32 height) const -> Memory::Dynamic::Bytes {
  Memory::Dynamic::Bytes inputs;
  inputs.append(0, description.host_size);
  auto access = inputs.get_access();
  // Parameters already use the generated target layout. Host roles fill only
  // values owned by current placement and presentation state.
  if (description.parameters_size != 0) {
    Data::copy(
        access.get_data() + description.parameters_offset,
        batch.get_inputs().get_data(), description.parameters_size);
  }

  const auto& transform = batch.get_transform();
  const auto size = batch.get_size_pixels();
  for (const Description::HostField& field : description.host_fields) {
    if (field.offset > description.host_size ||
        field.size > description.host_size - field.offset) {
      return {};
    }
    if (field.role == Description::HostRole::Parameter) {
      if (field.offset < description.parameters_offset ||
          field.offset + field.size >
              description.parameters_offset + description.parameters_size) {
        return {};
      }
      continue;
    }
    if (field.size != sizeof(R32) * 4) {
      return {};
    }
    R32 values[4] = {};
    if (field.role == Description::HostRole::TransformX) {
      values[0] = R32(transform.get_xx() * size.width);
      values[1] = R32(transform.get_xy() * size.height);
      values[2] = R32(transform.get_x());
      values[3] = R32(width);
    } else if (field.role == Description::HostRole::TransformY) {
      values[0] = R32(transform.get_yx() * size.width);
      values[1] = R32(transform.get_yy() * size.height);
      values[2] = R32(transform.get_y());
      values[3] = R32(height);
    }
    Data::copy(access.get_data() + field.offset, values, 4);
  }
  return inputs;
}

auto Vulkan::Pipelines::find_texture(
    const Perimortem::Graphics::Frame::Resource& resource) -> Texture* {
  for (Core::Object<> object : textures.get_view()) {
    TextureCacheEntry& entry = get_cache_entry(object);
    if (entry.resource.matches(resource)) {
      return &entry.texture;
    }
  }

  return nullptr;
}

auto Vulkan::Pipelines::find_image(
    const Perimortem::Graphics::Frame::Resource& resource) -> TextureImage* {
  for (Core::Object<> object : images.get_view()) {
    ImageCacheEntry& entry = get_image_cache_entry(object);
    if (entry.resource.get_object().get_payload() ==
        resource.get_object().get_payload()) {
      return &entry.image;
    }
  }

  return nullptr;
}

auto Vulkan::Pipelines::realize_image(
    const Perimortem::Graphics::Frame::Resource& resource) -> TextureImage* {
  TextureImage* retained = find_image(resource);
  if (retained) {
    return retained;
  }

  Core::Object<> storage = Core::Object<>::create(image_cache_descriptor);
  new (storage.get_payload(), Core::Placement::Construct) ImageCacheEntry();
  ImageCacheEntry& entry = get_image_cache_entry(storage);
  entry.resource = resource;
  entry.image = TextureImage::create(context, resource);
  Core::Object<>& inserted =
      images.emplace(static_cast<Core::Object<>&&>(storage));

  return &get_image_cache_entry(inserted).image;
}

auto Vulkan::Pipelines::realize_texture(
    const Perimortem::Graphics::Frame::Resource& resource) -> Texture* {
  Texture* retained = find_texture(resource);
  if (retained) {
    return retained;
  }

  TextureImage* image = find_image(resource);
  BAIL_IF(image == nullptr);
  Core::Object<> storage = Core::Object<>::create(cache_descriptor);
  new (storage.get_payload(), Core::Placement::Construct) TextureCacheEntry();
  TextureCacheEntry& entry = get_cache_entry(storage);
  entry.resource = resource;
  entry.texture = Texture::create(
      context, resource, image->get_view(), descriptor_layout);
  Core::Object<>& inserted =
      textures.emplace(static_cast<Core::Object<>&&>(storage));

  return &get_cache_entry(inserted).texture;
}

auto Vulkan::Pipelines::sweep_textures() -> void {
  Count index = 0;
  while (index < images.get_size()) {
    ImageCacheEntry& image = get_image_cache_entry(images[index]);
    U8* identity = image.resource.get_object().get_payload();
    Count cached_reservations = 1;
    for (Core::Object<> object : textures.get_view()) {
      const TextureCacheEntry& texture = get_cache_entry(object);
      cached_reservations +=
          texture.resource.get_object().get_payload() == identity;
    }
    if (image.resource.get_reservations() != cached_reservations) {
      index++;
      continue;
    }

    Count texture_index = 0;
    while (texture_index < textures.get_size()) {
      const TextureCacheEntry& texture =
          get_cache_entry(textures[texture_index]);
      if (texture.resource.get_object().get_payload() != identity) {
        texture_index++;
        continue;
      }

      textures[texture_index].release();
      textures.remove(texture_index);
    }
    images[index].release();
    images.remove(index);
  }
}

auto Vulkan::Pipelines::finalize_image_cache(U8* payload) -> void {
  Data::cast<ImageCacheEntry>(payload)->~ImageCacheEntry();
}

auto Vulkan::Pipelines::finalize_cache(U8* payload) -> void {
  Data::cast<TextureCacheEntry>(payload)->~TextureCacheEntry();
}

auto Vulkan::Pipelines::finalize_realization(U8* payload) -> void {
  Data::cast<Realization>(payload)->~Realization();
}

auto Vulkan::Pipelines::get_cache_entry(Core::Object<> object)
    -> TextureCacheEntry& {
  return *Data::cast<TextureCacheEntry>(object.get_payload());
}

auto Vulkan::Pipelines::get_image_cache_entry(Core::Object<> object)
    -> ImageCacheEntry& {
  return *Data::cast<ImageCacheEntry>(object.get_payload());
}

auto Vulkan::Pipelines::get_realization(Core::Object<> object) -> Realization& {
  return *Data::cast<Realization>(object.get_payload());
}
