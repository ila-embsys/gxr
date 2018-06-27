/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_VULKAN_TEXTURE_H_
#define OPENVR_GLIB_VULKAN_TEXTURE_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_VULKAN_TEXTURE openvr_vulkan_texture_get_type()
G_DECLARE_FINAL_TYPE (OpenVRVulkanTexture, openvr_vulkan_texture, OPENVR, VULKAN_TEXTURE, GObject)

struct _OpenVRVulkanTexture
{
  GObjectClass parent_class;
};

OpenVRVulkanTexture *openvr_vulkan_texture_new (void);

static void
openvr_vulkan_texture_finalize (GObject *gobject);


G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_TEXTURE_H_ */
