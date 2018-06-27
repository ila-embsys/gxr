/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-instance.h"

G_DEFINE_TYPE (OpenVRVulkanInstance, openvr_vulkan_instance, G_TYPE_OBJECT)

static void
openvr_vulkan_instance_class_init (OpenVRVulkanInstanceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_vulkan_instance_finalize;
}

static void
openvr_vulkan_instance_init (OpenVRVulkanInstance *self)
{
}

OpenVRVulkanInstance *
openvr_vulkan_instance_new (void)
{
  return (OpenVRVulkanInstance*) g_object_new (OPENVR_TYPE_VULKAN_INSTANCE, 0);
}

static void
openvr_vulkan_instance_finalize (GObject *gobject)
{
  // OpenVRVulkanInstance *self = OPENVR_VULKAN_INSTANCE (gobject);
}
