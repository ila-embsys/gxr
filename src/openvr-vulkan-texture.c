/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-texture.h"

G_DEFINE_TYPE (OpenVRVulkanTexture, openvr_vulkan_texture, G_TYPE_OBJECT)

static void
openvr_vulkan_texture_class_init (OpenVRVulkanTextureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_vulkan_texture_finalize;
}

static void
openvr_vulkan_texture_init (OpenVRVulkanTexture *self)
{
}

OpenVRVulkanTexture *
openvr_vulkan_texture_new (void)
{
  return (OpenVRVulkanTexture*) g_object_new (OPENVR_TYPE_VULKAN_TEXTURE, 0);
}

static void
openvr_vulkan_texture_finalize (GObject *gobject)
{
  // OpenVRVulkanTexture *self = OPENVR_VULKAN_TEXTURE (gobject);
}
