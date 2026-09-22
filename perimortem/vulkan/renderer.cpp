// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/vulkan/renderer.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem;

Vulkan::Renderer::Renderer(
    System::Presentation presentation,
    U32 width,
    U32 height)
    : context(Vulkan::Context::create(presentation)),
      swapchain(Vulkan::Swapchain::create(context, width, height)) {
  allocate_frames();
  refresh_swapchain_images();
}

Vulkan::Renderer::~Renderer() {
  wait_idle();
  destroy_frames();
}

auto Vulkan::Renderer::begin_frame(Frame& frame) -> Bool {
  FrameState& state = frames[current_frame];
  vkWaitForFences(context.get_device(), 1, &state.fence, VK_TRUE, UINT64_MAX);

  const U32 image_index = swapchain.acquire_next_image(state.image_available);
  if (image_index == UINT32_MAX) {
    return False;
  }

  vkResetFences(context.get_device(), 1, &state.fence);
  vkResetCommandBuffer(state.command_buffer, 0);

  frame.command_buffer = state.command_buffer;
  frame.width = swapchain.get_extent().width;
  frame.height = swapchain.get_extent().height;
  frame.frame_index = current_frame;
  frame.image_index = image_index;
  begin_rendering(frame);
  return True;
}

auto Vulkan::Renderer::end_frame(const Frame& frame) -> void {
  finish_rendering(frame);
  FrameState& state = frames[frame.frame_index];

  constexpr VkPipelineStageFlags wait_stage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.waitSemaphoreCount = 1;
  submit.pWaitSemaphores = &state.image_available;
  submit.pWaitDstStageMask = &wait_stage;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &state.command_buffer;
  submit.signalSemaphoreCount = 1;
  submit.pSignalSemaphores = &state.render_finished;
  require_success(
      vkQueueSubmit(context.get_graphics_queue(), 1, &submit, state.fence),
      "Vulkan::Renderer: Failed to submit frame."_view);

  const VkSwapchainKHR swapchain_handle = swapchain.get_swapchain();
  VkPresentInfoKHR present = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  present.waitSemaphoreCount = 1;
  present.pWaitSemaphores = &state.render_finished;
  present.swapchainCount = 1;
  present.pSwapchains = &swapchain_handle;
  present.pImageIndices = &frame.image_index;

  const VkResult present_result =
      vkQueuePresentKHR(context.get_graphics_queue(), &present);
  if (present_result != VK_SUCCESS && present_result != VK_SUBOPTIMAL_KHR &&
      present_result != VK_ERROR_OUT_OF_DATE_KHR) {
    Diagnostics::Log::fatal("Vulkan::Renderer: Failed to present frame."_view);
  }

  current_frame = (current_frame + 1) % frames_in_flight;
}

auto Vulkan::Renderer::wait_idle() -> void {
  if (context.get_device()) {
    vkDeviceWaitIdle(context.get_device());
  }
}

auto Vulkan::Renderer::get_width() const -> U32 {
  return swapchain.get_extent().width;
}

auto Vulkan::Renderer::get_height() const -> U32 {
  return swapchain.get_extent().height;
}

auto Vulkan::Renderer::get_context() const -> const Vulkan::Context& {
  return context;
}

auto Vulkan::Renderer::get_swapchain() const -> const Vulkan::Swapchain& {
  return swapchain;
}

auto Vulkan::Renderer::require_success(VkResult result, View::Bytes message)
    -> void {
  if (result != VK_SUCCESS) {
    Diagnostics::Log::fatal(message);
  }
}

auto Vulkan::Renderer::resize(U32 width, U32 height) -> Bool {
  wait_idle();
  const VkFormat old_format = swapchain.get_format();
  swapchain.recreate(context, width, height);
  refresh_swapchain_images();
  return swapchain.get_format() != old_format;
}

auto Vulkan::Renderer::allocate_frames() -> void {
  VkCommandBufferAllocateInfo allocation = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocation.commandPool = context.get_command_pool();
  allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocation.commandBufferCount = 1;
  for (Count i = 0; i < frames_in_flight; i++) {
    require_success(
        vkAllocateCommandBuffers(
            context.get_device(), &allocation, &frames[i].command_buffer),
        "Vulkan::Renderer: Failed to allocate command buffer."_view);

    VkSemaphoreCreateInfo semaphore_info = {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    require_success(
        vkCreateSemaphore(
            context.get_device(), &semaphore_info, nullptr,
            &frames[i].image_available),
        "Vulkan::Renderer: Failed to create image-available semaphore."_view);
    require_success(
        vkCreateSemaphore(
            context.get_device(), &semaphore_info, nullptr,
            &frames[i].render_finished),
        "Vulkan::Renderer: Failed to create render-finished semaphore."_view);

    VkFenceCreateInfo fence_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    require_success(
        vkCreateFence(
            context.get_device(), &fence_info, nullptr, &frames[i].fence),
        "Vulkan::Renderer: Failed to create frame fence."_view);
  }
}

auto Vulkan::Renderer::destroy_frames() -> void {
  for (Count i = 0; i < frames_in_flight; i++) {
    if (frames[i].fence) {
      vkDestroyFence(context.get_device(), frames[i].fence, nullptr);
    }

    if (frames[i].render_finished) {
      vkDestroySemaphore(
          context.get_device(), frames[i].render_finished, nullptr);
    }

    if (frames[i].image_available) {
      vkDestroySemaphore(
          context.get_device(), frames[i].image_available, nullptr);
    }

    frames[i] = {};
  }
}

auto Vulkan::Renderer::refresh_swapchain_images() -> void {
  for (U32 i = 0; i < swapchain.get_image_count(); i++) {
    swapchain_images[i] = swapchain.get_image(i);
  }
}

auto Vulkan::Renderer::begin_rendering(const Frame& frame) -> void {
  VkCommandBufferBeginInfo begin_info = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  require_success(
      vkBeginCommandBuffer(frame.command_buffer, &begin_info),
      "Vulkan::Renderer: Failed to begin command buffer."_view);

  VkImageMemoryBarrier2 to_attachment = {
    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
  to_attachment.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  to_attachment.srcAccessMask = VkAccessFlags2(0);
  to_attachment.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  to_attachment.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  to_attachment.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  to_attachment.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  to_attachment.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  to_attachment.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  to_attachment.image = swapchain_images[frame.image_index];
  to_attachment.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  VkDependencyInfo attachment_dependency = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
  attachment_dependency.imageMemoryBarrierCount = 1;
  attachment_dependency.pImageMemoryBarriers = &to_attachment;
  vkCmdPipelineBarrier2(frame.command_buffer, &attachment_dependency);

  VkRenderingAttachmentInfo color_attachment = {
    VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
  color_attachment.imageView = swapchain.get_image_view(frame.image_index);
  color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

  const VkExtent2D extent = {frame.width, frame.height};
  VkRenderingInfo rendering = {VK_STRUCTURE_TYPE_RENDERING_INFO};
  rendering.renderArea = {{0, 0}, extent};
  rendering.layerCount = 1;
  rendering.colorAttachmentCount = 1;
  rendering.pColorAttachments = &color_attachment;
  vkCmdBeginRendering(frame.command_buffer, &rendering);

  const VkViewport viewport = {0.0f, 0.0f, R32(frame.width), R32(frame.height),
                               0.0f, 1.0f};
  vkCmdSetViewport(frame.command_buffer, 0, 1, &viewport);
  const VkRect2D scissor = {{0, 0}, extent};
  vkCmdSetScissor(frame.command_buffer, 0, 1, &scissor);
}

auto Vulkan::Renderer::finish_rendering(const Frame& frame) -> void {
  vkCmdEndRendering(frame.command_buffer);

  VkImageMemoryBarrier2 to_present = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
  to_present.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  to_present.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  to_present.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
  to_present.dstAccessMask = VkAccessFlags2(0);
  to_present.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  to_present.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  to_present.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  to_present.image = swapchain_images[frame.image_index];
  to_present.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  VkDependencyInfo present_dependency = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
  present_dependency.imageMemoryBarrierCount = 1;
  present_dependency.pImageMemoryBarriers = &to_present;
  vkCmdPipelineBarrier2(frame.command_buffer, &present_dependency);
  require_success(
      vkEndCommandBuffer(frame.command_buffer),
      "Vulkan::Renderer: Failed to end command buffer."_view);
}
