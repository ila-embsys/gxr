/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_VULKAN_DEVICE_H_
#define OPENVR_GLIB_VULKAN_DEVICE_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_VULKAN_DEVICE openvr_vulkan_device_get_type()
G_DECLARE_FINAL_TYPE (OpenVRVulkanDevice, openvr_vulkan_device, OPENVR, VULKAN_DEVICE, GObject)

struct _OpenVRVulkanDevice
{
  GObjectClass parent_class;
};

OpenVRVulkanDevice *openvr_vulkan_device_new (void);

static void
openvr_vulkan_device_finalize (GObject *gobject);


G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_DEVICE_H_ */
