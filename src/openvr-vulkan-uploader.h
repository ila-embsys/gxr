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

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#include <openvr_capi.h>

#include "openvr-vulkan-client.h"
#include "openvr-vulkan-texture.h"
#include "openvr-overlay.h"
#include "openvr-system.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_VULKAN_UPLOADER openvr_vulkan_uploader_get_type()
G_DECLARE_FINAL_TYPE (OpenVRVulkanUploader, openvr_vulkan_uploader,
                      OPENVR, VULKAN_UPLOADER, OpenVRVulkanClient)

struct _OpenVRVulkanUploader
{
  OpenVRVulkanClient parent_type;
};

struct _OpenVRVulkanUploaderClass
{
  GObjectClass parent_class;
};

OpenVRVulkanUploader *openvr_vulkan_uploader_new (void);

bool
openvr_vulkan_uploader_init_vulkan (OpenVRVulkanUploader *self,
                                    bool enable_validation);

void
openvr_vulkan_uploader_submit_frame (OpenVRVulkanUploader *self,
                                     OpenVROverlay        *overlay,
                                     OpenVRVulkanTexture  *texture);

bool
openvr_vulkan_uploader_load_dmabuf (OpenVRVulkanUploader *self,
                                    OpenVRVulkanTexture  *texture,
                                    int                   fd,
                                    guint                 width,
                                    guint                 height,
                                    VkFormat              format);

G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_UPLOADER_H_ */
