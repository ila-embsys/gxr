/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_VULKAN_MODEL_MANAGER_H_
#define OPENVR_GLIB_VULKAN_MODEL_MANAGER_H_

#include <glib-object.h>

#include "xrd-scene-device.h"
#include <gulkan-client.h>

#define MAX_TRACKED_DEVICES 64

G_BEGIN_DECLS

#define OPENVR_TYPE_VULKAN_MODEL_MANAGER openvr_vulkan_model_manager_get_type()
G_DECLARE_FINAL_TYPE (OpenVRVulkanModelManager, openvr_vulkan_model_manager,
                      OPENVR, VULKAN_MODEL_MANAGER, GObject)

struct _OpenVRVulkanModelManager
{
  GObject parent;

  GHashTable *model_content;

  OpenVRVulkanModel *models[MAX_TRACKED_DEVICES];
};

OpenVRVulkanModelManager *openvr_vulkan_model_manager_new (void);

void
openvr_vulkan_model_manager_load (OpenVRVulkanModelManager *self,
                                  GulkanClient             *client,
                                  TrackedDeviceIndex_t      device_id,
                                  VkDescriptorSetLayout    *layout);

G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_MODEL_MANAGER_H_ */
