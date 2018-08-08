/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_VULKAN_RENDERER_H_
#define OPENVR_GLIB_VULKAN_RENDERER_H_

#include <stdint.h>
#include <glib-object.h>

#include "openvr-vulkan-client.h"
#include "openvr-vulkan-texture.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_VULKAN_RENDERER openvr_vulkan_renderer_get_type()
G_DECLARE_FINAL_TYPE (OpenVRVulkanRenderer, openvr_vulkan_renderer,
                      OPENVR, VULKAN_RENDERER, OpenVRVulkanClient)

struct _OpenVRVulkanRenderer
{
  OpenVRVulkanClient parent_type;

  VkSwapchainKHR swap_chain;
  VkImageView *swapchain_image_views;
  VkFormat swapchain_image_format;
  uint32_t swapchain_image_count;
  VkExtent2D swapchain_extent;

  VkRenderPass render_pass;
  VkDescriptorSetLayout descriptor_set_layout;

  VkPipeline graphics_pipeline;
  VkPipelineLayout pipeline_layout;

  VkFramebuffer *framebuffers;

  VkBuffer *uniform_buffers;
  VkDeviceMemory *uniform_buffers_memory;

  VkDescriptorPool descriptor_pool;

  VkDescriptorSet *descriptor_sets;

  VkBuffer vertex_buffer;
  VkDeviceMemory vertex_buffer_memory;
  VkBuffer index_buffer;
  VkDeviceMemory index_buffer_memory;

  VkSemaphore *image_available_semaphores;
  VkSemaphore *render_finished_semaphores;
  VkFence *in_flight_fences;

  VkCommandBuffer *draw_cmd_buffers;

  VkSampler sampler;

  size_t current_frame;
};

struct _OpenVRVulkanRendererClass
{
  OpenVRVulkanClientClass parent_class;
};

OpenVRVulkanRenderer *openvr_vulkan_renderer_new (void);


bool
openvr_vulkan_renderer_init_rendering (OpenVRVulkanRenderer *self,
                                       VkSurfaceKHR surface,
                                       OpenVRVulkanTexture *texture);

bool
openvr_vulkan_renderer_draw (OpenVRVulkanRenderer *self);

G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_RENDERER_H_ */
