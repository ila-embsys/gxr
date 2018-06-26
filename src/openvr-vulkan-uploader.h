/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_VULKAN_UPLOADER_H_
#define OPENVR_GLIB_VULKAN_UPLOADER_H_

#include <stdint.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#include <openvr_capi.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_VULKAN_UPLOADER openvr_vulkan_uploader_get_type()
G_DECLARE_FINAL_TYPE (OpenVRVulkanUploader, openvr_vulkan_uploader,
                      OPENVR, VULKAN_UPLOADER, GObject)

typedef struct
{
  VkCommandBuffer cmd_buffer;
  VkFence fence;
} VulkanCommandBuffer_t;

struct _OpenVRVulkanUploader
{
  GObjectClass parent_class;

  bool enable_validation;

  VkInstance instance;
  VkDevice device;
  VkPhysicalDevice physical_device;
  VkQueue queue;
  VkDebugReportCallbackEXT debug_report_cb;
  VkCommandPool command_pool;
  VkImage image;
  VkDeviceMemory image_memory;
  VkImageView image_view;
  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;

  VROverlayHandle_t overlay_handle;
  VROverlayHandle_t thumbnail_handle;

  VkPhysicalDeviceProperties physical_device_properties;
  VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
  VkPhysicalDeviceFeatures physical_device_features;

  uint32_t queue_family_index;

  VulkanCommandBuffer_t *current_cmd_buffer;

  GQueue * cmd_buffers;

  uint32_t width;
  uint32_t height;

  struct VR_IVRSystem_FnTable *system;
  struct VR_IVRCompositor_FnTable *compositor;
  struct VR_IVROverlay_FnTable *overlay;
};

OpenVRVulkanUploader *openvr_vulkan_uploader_new (void);

bool openvr_vulkan_uploader_init_vulkan (OpenVRVulkanUploader *self);
bool openvr_vulkan_uploader_init_vulkan_instance (OpenVRVulkanUploader *self);
bool openvr_vulkan_uploader_init_vulkan_device (OpenVRVulkanUploader *self);
void openvr_vulkan_uploader_shutdown (OpenVRVulkanUploader *self);
void openvr_vulkan_uploader_submit_frame (OpenVRVulkanUploader *self);
bool openvr_vulkan_uploader_init_texture (OpenVRVulkanUploader *self);

VulkanCommandBuffer_t* openvr_vulkan_uploader_get_command_buffer (
  OpenVRVulkanUploader *self);

G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_UPLOADER_H_ */
