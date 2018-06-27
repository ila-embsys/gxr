/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-device.h"

G_DEFINE_TYPE (OpenVRVulkanDevice, openvr_vulkan_device, G_TYPE_OBJECT)

static void
openvr_vulkan_device_class_init (OpenVRVulkanDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_vulkan_device_finalize;
}

static void
openvr_vulkan_device_init (OpenVRVulkanDevice *self)
{
}

OpenVRVulkanDevice *
openvr_vulkan_device_new (void)
{
  return (OpenVRVulkanDevice*) g_object_new (OPENVR_TYPE_VULKAN_DEVICE, 0);
}

static void
openvr_vulkan_device_finalize (GObject *gobject)
{
  // OpenVRVulkanDevice *self = OPENVR_VULKAN_DEVICE (gobject);
}
