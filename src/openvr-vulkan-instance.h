/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_VULKAN_INSTANCE_H_
#define OPENVR_GLIB_VULKAN_INSTANCE_H_

#include <glib-object.h>
#include <vulkan/vulkan.h>
#include <stdbool.h>

#include "openvr-compositor.h"

#define ENUM_TO_STR(r) case r: return #r

G_BEGIN_DECLS

#define OPENVR_TYPE_VULKAN_INSTANCE openvr_vulkan_instance_get_type()
G_DECLARE_FINAL_TYPE (OpenVRVulkanInstance, openvr_vulkan_instance, OPENVR, VULKAN_INSTANCE, GObject)

struct _OpenVRVulkanInstance
{
  GObjectClass parent_class;

  bool enable_validation;

  VkInstance instance;
  VkDebugReportCallbackEXT debug_report_cb;
};

OpenVRVulkanInstance *openvr_vulkan_instance_new (void);

bool
openvr_vulkan_instance_create (OpenVRVulkanInstance *self,
                               bool enable_validation,
                               OpenVRCompositor *compositor);

G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_INSTANCE_H_ */
