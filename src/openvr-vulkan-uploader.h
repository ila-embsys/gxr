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

#include "openvr-vulkan-instance.h"
#include "openvr-vulkan-device.h"
#include "openvr-vulkan-texture.h"
#include "openvr-compositor.h"
#include "openvr-overlay.h"

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

  VkCommandPool command_pool;

  OpenVRVulkanInstance *instance;
  OpenVRVulkanDevice *device;

  VulkanCommandBuffer_t *current_cmd_buffer;

  GQueue * cmd_buffers;
};

OpenVRVulkanUploader *openvr_vulkan_uploader_new (void);

bool
openvr_vulkan_uploader_init_vulkan (OpenVRVulkanUploader *self,
                                    bool enable_validation,
                                    OpenVRSystem *system,
                                    OpenVRCompositor *compositor);


void
openvr_vulkan_uploader_submit_frame (OpenVRVulkanUploader *self,
                                     OpenVROverlay        *overlay,
                                     OpenVRVulkanTexture  *texture);

bool
openvr_vulkan_uploader_load_texture_raw (OpenVRVulkanUploader *self,
                                         OpenVRVulkanTexture  *texture,
                                         guchar               *pixels,
                                         guint                 width,
                                         guint                 height,
                                         gsize                 size);

G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_UPLOADER_H_ */
