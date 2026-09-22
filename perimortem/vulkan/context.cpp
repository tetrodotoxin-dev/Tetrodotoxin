// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/vulkan/context.hpp"

#include <vulkan/vulkan_wayland.h>
#include <wayland-client.h>

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

using namespace Perimortem::Core;
using namespace Perimortem;

constexpr Static::Vector<const char*, 2> instance_extensions = {{
  VK_KHR_SURFACE_EXTENSION_NAME,
  VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
}};

constexpr Static::Vector<const char*, 1> device_extensions = {
  {VK_KHR_SWAPCHAIN_EXTENSION_NAME}};

#ifdef PERI_DEBUG
constexpr Static::Vector<const char*, 1> validation_layers = {
  {"VK_LAYER_KHRONOS_validation"}};
#endif

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

#ifdef PERI_DEBUG
static auto instance_layer_available(View::Bytes layer_name) -> Bool {
  U32 layer_count = 0;
  if (vkEnumerateInstanceLayerProperties(&layer_count, nullptr) != VK_SUCCESS) {
    return False;
  }

  if (layer_count == 0) {
    return False;
  }

  Perimortem::Memory::Dynamic::Vector<VkLayerProperties> layers;
  layers.forgetful_resize(layer_count);
  U32 read_layer_count = layer_count;
  const VkResult layer_read_result =
      vkEnumerateInstanceLayerProperties(&read_layer_count, layers.get_data());
  if (layer_read_result != VK_SUCCESS && layer_read_result != VK_INCOMPLETE) {
    return False;
  }

  return layers.get_view()
      .slice(0, read_layer_count)
      .contains([layer_name](const VkLayerProperties& layer) {
        return NullTerminated::to_view(layer.layerName) == layer_name;
      });
}
#endif

static auto select_physical_device(VkInstance instance, VkSurfaceKHR surface)
    -> VkPhysicalDevice {
  U32 physical_device_count = 0;
  require_success(
      vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr),
      "Vulkan: Failed to query physical devices."_view);
  if (physical_device_count == 0) {
    return VK_NULL_HANDLE;
  }

  Perimortem::Memory::Dynamic::Vector<VkPhysicalDevice> physical_devices;
  physical_devices.forgetful_resize(physical_device_count);
  U32 read_physical_device_count = physical_device_count;
  require_enumeration_read(
      vkEnumeratePhysicalDevices(
          instance, &read_physical_device_count, physical_devices.get_data()),
      "Vulkan: Failed to read physical devices."_view);
  if (read_physical_device_count == 0) {
    return VK_NULL_HANDLE;
  }

  // Prefer a discrete GPU when one can present to this surface. Integrated and
  // software devices remain valid fallbacks so development and inspection do
  // not depend on a particular machine configuration.
  VkPhysicalDevice fallback = VK_NULL_HANDLE;
  for (U32 i = 0; i < read_physical_device_count; i++) {
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

    // Check that a queue family supports both graphics and present.
    U32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_devices[i], &queue_family_count, nullptr);
    if (queue_family_count == 0) {
      continue;
    }

    Perimortem::Memory::Dynamic::Vector<VkQueueFamilyProperties> queue_families;
    queue_families.forgetful_resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_devices[i], &queue_family_count, queue_families.get_data());

    Bool usable = False;
    for (U32 queue_family = 0; queue_family < queue_family_count;
         queue_family++) {
      if (!(queue_families[queue_family].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
        continue;
      }

      VkBool32 present_supported = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(
          physical_devices[i], queue_family, surface, &present_supported);
      if (present_supported) {
        usable = True;
        break;
      }
    }

    if (!usable) {
      continue;
    }

    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      return physical_devices[i];
    }

    fallback = physical_devices[i];
  }

  return fallback;
}

static auto find_graphics_queue_family(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface) -> U32 {
  U32 queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &queue_family_count, nullptr);
  if (queue_family_count == 0) {
    return UINT32_MAX;
  }

  Perimortem::Memory::Dynamic::Vector<VkQueueFamilyProperties> queue_families;
  queue_families.forgetful_resize(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &queue_family_count, queue_families.get_data());
  for (U32 i = 0; i < queue_family_count; i++) {
    if (!(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      continue;
    }

    VkBool32 present = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present);
    if (present) {
      return i;
    }
  }

  return UINT32_MAX;
}

auto Vulkan::Context::create(System::Presentation presentation)
    -> Vulkan::Context {
  if (!presentation.is_valid() ||
      presentation.get_kind() != System::Presentation::Kind::Wayland) {
    Diagnostics::Log::fatal(
        "Vulkan: The selected presentation kind is not supported."_view);
  }

  Vulkan::Context ctx;

  VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
  app_info.pApplicationName = "Perimortem";
  app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  instance_info.pApplicationInfo = &app_info;
  instance_info.enabledExtensionCount = U32(instance_extensions.get_size());
  instance_info.ppEnabledExtensionNames = instance_extensions.get_data();

#ifdef PERI_DEBUG
  if (instance_layer_available("VK_LAYER_KHRONOS_validation"_view)) {
    instance_info.enabledLayerCount = U32(validation_layers.get_size());
    instance_info.ppEnabledLayerNames = validation_layers.get_data();
  }
#endif

  require_success(
      vkCreateInstance(&instance_info, nullptr, &ctx.instance),
      "Vulkan: Failed to create instance."_view);

  VkWaylandSurfaceCreateInfoKHR surface_info = {
    VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
  surface_info.display = static_cast<wl_display*>(presentation.get_host());
  surface_info.surface = static_cast<wl_surface*>(presentation.get_surface());
  require_success(
      vkCreateWaylandSurfaceKHR(
          ctx.instance, &surface_info, nullptr, &ctx.surface),
      "Vulkan: Failed to create window surface."_view);
  if (!ctx.surface) {
    Diagnostics::Log::fatal("Vulkan: Failed to create window surface."_view);
  }

  ctx.physical_device = select_physical_device(ctx.instance, ctx.surface);
  if (!ctx.physical_device) {
    Diagnostics::Log::fatal("Vulkan: No usable physical device found."_view);
  }

  ctx.graphics_queue_family =
      find_graphics_queue_family(ctx.physical_device, ctx.surface);
  if (ctx.graphics_queue_family == UINT32_MAX) {
    Diagnostics::Log::fatal(
        "Vulkan: No graphics queue family can present to the surface."_view);
  }

  constexpr R32 queue_priority = 1.0f;
  VkDeviceQueueCreateInfo queue_info = {
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queue_info.queueFamilyIndex = ctx.graphics_queue_family;
  queue_info.queueCount = 1;
  queue_info.pQueuePriorities = &queue_priority;

  VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES};
  dynamic_rendering.dynamicRendering = VK_TRUE;

  VkPhysicalDeviceSynchronization2Features sync2 = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES};
  sync2.synchronization2 = VK_TRUE;
  sync2.pNext = &dynamic_rendering;

  VkPhysicalDeviceFeatures available_features = {};
  vkGetPhysicalDeviceFeatures(ctx.physical_device, &available_features);
  VkPhysicalDeviceFeatures enabled_features = {};
  enabled_features.shaderFloat64 = available_features.shaderFloat64;
  ctx.float64 = Bool(available_features.shaderFloat64);

  VkDeviceCreateInfo device_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  device_info.pNext = &sync2;
  device_info.queueCreateInfoCount = 1;
  device_info.pQueueCreateInfos = &queue_info;
  device_info.enabledExtensionCount = U32(device_extensions.get_size());
  device_info.ppEnabledExtensionNames = device_extensions.get_data();
  device_info.pEnabledFeatures = &enabled_features;

  require_success(
      vkCreateDevice(ctx.physical_device, &device_info, nullptr, &ctx.device),
      "Vulkan: Failed to create logical device."_view);
  vkGetDeviceQueue(
      ctx.device, ctx.graphics_queue_family, 0, &ctx.graphics_queue);

  VkCommandPoolCreateInfo pool_info = {
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = ctx.graphics_queue_family;
  require_success(
      vkCreateCommandPool(ctx.device, &pool_info, nullptr, &ctx.command_pool),
      "Vulkan: Failed to create command pool."_view);
  return ctx;
}

Vulkan::Context::~Context() {
  if (!device) {
    return;
  }

  vkDestroyCommandPool(device, command_pool, nullptr);
  vkDestroyDevice(device, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);
}

Vulkan::Context::Context(Vulkan::Context&& other) noexcept
    : instance(other.instance),
      surface(other.surface),
      physical_device(other.physical_device),
      device(other.device),
      graphics_queue(other.graphics_queue),
      graphics_queue_family(other.graphics_queue_family),
      command_pool(other.command_pool),
      float64(other.float64) {
  other.instance = VK_NULL_HANDLE;
  other.surface = VK_NULL_HANDLE;
  other.physical_device = VK_NULL_HANDLE;
  other.device = VK_NULL_HANDLE;
  other.graphics_queue = VK_NULL_HANDLE;
  other.graphics_queue_family = 0;
  other.command_pool = VK_NULL_HANDLE;
  other.float64 = False;
}

auto Vulkan::Context::operator=(Vulkan::Context&& other) noexcept
    -> Vulkan::Context& {
  if (this != &other) {
    this->~Context();
    instance = other.instance;
    surface = other.surface;
    physical_device = other.physical_device;
    device = other.device;
    graphics_queue = other.graphics_queue;
    graphics_queue_family = other.graphics_queue_family;
    command_pool = other.command_pool;
    float64 = other.float64;
    other.instance = VK_NULL_HANDLE;
    other.surface = VK_NULL_HANDLE;
    other.physical_device = VK_NULL_HANDLE;
    other.device = VK_NULL_HANDLE;
    other.graphics_queue = VK_NULL_HANDLE;
    other.graphics_queue_family = 0;
    other.command_pool = VK_NULL_HANDLE;
    other.float64 = False;
  }

  return *this;
}

auto Vulkan::Context::get_instance() const -> VkInstance {
  return instance;
}

auto Vulkan::Context::get_physical_device() const -> VkPhysicalDevice {
  return physical_device;
}

auto Vulkan::Context::get_device() const -> VkDevice {
  return device;
}

auto Vulkan::Context::get_surface() const -> VkSurfaceKHR {
  return surface;
}

auto Vulkan::Context::get_graphics_queue() const -> VkQueue {
  return graphics_queue;
}

auto Vulkan::Context::get_graphics_queue_family() const -> U32 {
  return graphics_queue_family;
}

auto Vulkan::Context::get_command_pool() const -> VkCommandPool {
  return command_pool;
}

auto Vulkan::Context::supports_float64() const -> Bool {
  return float64;
}

auto Vulkan::Context::begin_immediate_commands() const -> VkCommandBuffer {
  VkCommandBufferAllocateInfo allocation = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocation.commandPool = command_pool;
  allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocation.commandBufferCount = 1;

  VkCommandBuffer command_buffer = VK_NULL_HANDLE;
  require_success(
      vkAllocateCommandBuffers(device, &allocation, &command_buffer),
      "Vulkan: Failed to allocate immediate command buffer."_view);

  VkCommandBufferBeginInfo begin_info = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  require_success(
      vkBeginCommandBuffer(command_buffer, &begin_info),
      "Vulkan: Failed to begin immediate command buffer."_view);
  return command_buffer;
}

auto Vulkan::Context::submit_immediate_commands(
    VkCommandBuffer command_buffer) const -> void {
  require_success(
      vkEndCommandBuffer(command_buffer),
      "Vulkan: Failed to end immediate command buffer."_view);

  VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &command_buffer;
  require_success(
      vkQueueSubmit(graphics_queue, 1, &submit, VK_NULL_HANDLE),
      "Vulkan: Failed to submit immediate command buffer."_view);
  require_success(
      vkQueueWaitIdle(graphics_queue),
      "Vulkan: Failed to wait for immediate command buffer."_view);
  vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

auto Vulkan::Context::find_memory_type(
    U32 type_filter,
    VkMemoryPropertyFlags properties) const -> U32 {
  VkPhysicalDeviceMemoryProperties memory_properties = {};
  vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
  for (U32 i = 0; i < memory_properties.memoryTypeCount; i++) {
    const Bool type_matches = Bool((type_filter & (1u << i)) != 0);
    const Bool properties_match = Bool(
        (memory_properties.memoryTypes[i].propertyFlags & properties) ==
        properties);
    if (type_matches && properties_match) {
      return i;
    }
  }

  return UINT32_MAX;
}
