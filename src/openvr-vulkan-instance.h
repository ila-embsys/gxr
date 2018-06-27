/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_VULKAN_INSTANCE_H_
#define OPENVR_GLIB_VULKAN_INSTANCE_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_VULKAN_INSTANCE openvr_vulkan_instance_get_type()
G_DECLARE_FINAL_TYPE (OpenVRVulkanInstance, openvr_vulkan_instance, OPENVR, VULKAN_INSTANCE, GObject)

struct _OpenVRVulkanInstance
{
  GObjectClass parent_class;
};

OpenVRVulkanInstance *openvr_vulkan_instance_new (void);

static void
openvr_vulkan_instance_finalize (GObject *gobject);


G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_INSTANCE_H_ */
