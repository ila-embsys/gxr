/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-client.h"

G_DEFINE_TYPE (OpenVRVulkanClient, openvr_vulkan_client, G_TYPE_OBJECT)

static void
openvr_vulkan_client_finalize (GObject *gobject);

static void
openvr_vulkan_client_class_init (OpenVRVulkanClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_vulkan_client_finalize;
}

static void
openvr_vulkan_client_init (OpenVRVulkanClient *self)
{
  self->instance = openvr_vulkan_instance_new ();
  self->device = openvr_vulkan_device_new ();
  self->command_pool = VK_NULL_HANDLE;
  self->cmd_buffers = g_queue_new ();
}

OpenVRVulkanClient *
openvr_vulkan_client_new (void)
{
  return (OpenVRVulkanClient*) g_object_new (OPENVR_TYPE_VULKAN_CLIENT, 0);
}

static void
openvr_vulkan_client_finalize (GObject *gobject)
{
  // OpenVRVulkanClient *self = OPENVR_VULKAN_CLIENT (gobject);
}
