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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#include <openvr_capi.h>

#include "openvr-vulkan-instance.h"
#include "openvr-vulkan-device.h"
#include "openvr-vulkan-texture.h"
#include "openvr-compositor.h"
#include "openvr-overlay.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_VULKAN_RENDERER openvr_vulkan_renderer_get_type()
G_DECLARE_FINAL_TYPE (OpenVRVulkanRenderer, openvr_vulkan_renderer,
                      OPENVR, VULKAN_RENDERER, GObject)

typedef struct
{
  VkCommandBuffer cmd_buffer;
  VkFence fence;
} VulkanCommandBuffer_t;

struct _OpenVRVulkanRenderer
{
  GObjectClass parent_class;

  VkCommandPool command_pool;

  OpenVRVulkanInstance *instance;
  OpenVRVulkanDevice *device;

  VulkanCommandBuffer_t *current_cmd_buffer;

  GQueue * cmd_buffers;
};

OpenVRVulkanRenderer *openvr_vulkan_renderer_new (void);

bool
openvr_vulkan_renderer_init_vulkan (OpenVRVulkanRenderer *self,
                                    VkSurfaceKHR surface,
                                    bool enable_validation);

bool
openvr_vulkan_renderer_init_swapchain (VkDevice device,
                                       VkPhysicalDevice physical_device,
                                       VkSurfaceKHR surface);

void
openvr_vulkan_renderer_submit_frame (OpenVRVulkanRenderer *self,
                                     OpenVROverlay        *overlay,
                                     OpenVRVulkanTexture  *texture);

bool
openvr_vulkan_renderer_load_raw (OpenVRVulkanRenderer *self,
                                 OpenVRVulkanTexture  *texture,
                                 guchar               *pixels,
                                 guint                 width,
                                 guint                 height,
                                 gsize                 size,
                                 VkFormat              format);

bool
openvr_vulkan_renderer_load_pixbuf (OpenVRVulkanRenderer *self,
                                    OpenVRVulkanTexture  *texture,
                                    GdkPixbuf *pixbuf);

bool
openvr_vulkan_renderer_load_cairo_surface (OpenVRVulkanRenderer *self,
                                           OpenVRVulkanTexture  *texture,
                                           cairo_surface_t *surface);

bool
openvr_vulkan_renderer_load_dmabuf2 (OpenVRVulkanRenderer *self,
                                     OpenVRVulkanTexture  *texture,
                                     int                   fd,
                                     guint                 width,
                                     guint                 height,
                                     VkFormat              format);

G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_RENDERER_H_ */
