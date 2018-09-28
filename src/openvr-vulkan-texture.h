/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_VULKAN_TEXTURE_H_
#define OPENVR_GLIB_VULKAN_TEXTURE_H_

#include <glib-object.h>
#include <vulkan/vulkan.h>
#include <cairo.h>
#include <stdbool.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "openvr-vulkan-device.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_VULKAN_TEXTURE openvr_vulkan_texture_get_type()
G_DECLARE_FINAL_TYPE (OpenVRVulkanTexture, openvr_vulkan_texture,
                      OPENVR, VULKAN_TEXTURE, GObject)

struct _OpenVRVulkanTexture
{
  GObjectClass parent_class;

  VkImage image;
  VkDeviceMemory image_memory;
  VkImageView image_view;
  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;

  uint32_t width;
  uint32_t height;

  OpenVRVulkanDevice  *device;
};

OpenVRVulkanTexture *
openvr_vulkan_texture_new (OpenVRVulkanDevice  *device,
                           guint                width,
                           guint                height,
                           gsize                size,
                           VkFormat             format);

OpenVRVulkanTexture *
openvr_vulkan_texture_new_from_pixbuf (OpenVRVulkanDevice  *device,
                                       GdkPixbuf           *pixbuf);

OpenVRVulkanTexture *
openvr_vulkan_texture_new_from_cairo_surface (OpenVRVulkanDevice  *device,
                                              cairo_surface_t     *surface);

OpenVRVulkanTexture *
openvr_vulkan_texture_new_from_dmabuf (OpenVRVulkanDevice  *device,
                                       int                  fd,
                                       guint                width,
                                       guint                height,
                                       VkFormat             format);

void
openvr_vulkan_texture_transfer_layout (OpenVRVulkanTexture *self,
                                       OpenVRVulkanDevice  *device,
                                       VkCommandBuffer      cmd_buffer,
                                       VkImageLayout        old,
                                       VkImageLayout        new);

bool
openvr_vulkan_texture_upload_pixels (OpenVRVulkanTexture *self,
                                     VkCommandBuffer      cmd_buffer,
                                     guchar              *pixels,
                                     gsize                size);



void
openvr_vulkan_texture_free_staging_memory (OpenVRVulkanTexture *self);

G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_TEXTURE_H_ */
