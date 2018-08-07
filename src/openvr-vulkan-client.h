/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_VULKAN_CLIENT_H_
#define OPENVR_GLIB_VULKAN_CLIENT_H_

#include <glib-object.h>

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#include "openvr-vulkan-instance.h"
#include "openvr-vulkan-device.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_VULKAN_CLIENT openvr_vulkan_client_get_type()
G_DECLARE_FINAL_TYPE (OpenVRVulkanClient, openvr_vulkan_client,
                      OPENVR, VULKAN_CLIENT, GObject)

typedef struct
{
  VkCommandBuffer cmd_buffer;
  VkFence fence;
} VulkanCommandBuffer_t;

struct _OpenVRVulkanClient
{
  GObjectClass parent_class;

  VkCommandPool command_pool;

  OpenVRVulkanInstance *instance;
  OpenVRVulkanDevice *device;

  VulkanCommandBuffer_t *current_cmd_buffer;

  GQueue *cmd_buffers;
};

OpenVRVulkanClient *openvr_vulkan_client_new (void);


G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_CLIENT_H_ */
