/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_VULKAN_MODEL_H_
#define OPENVR_GLIB_VULKAN_MODEL_H_

#include <glib-object.h>

#include <gulkan-texture.h>
#include "gulkan-vertex-buffer.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_VULKAN_MODEL openvr_vulkan_model_get_type()
G_DECLARE_FINAL_TYPE (OpenVRVulkanModel, openvr_vulkan_model,
                      OPENVR, VULKAN_MODEL, GObject)

struct _OpenVRVulkanModel
{
  GObject parent;

  GulkanDevice *device;

  GulkanTexture *texture;
  GulkanVertexBuffer *vbo;
  VkSampler sampler;
};

OpenVRVulkanModel *openvr_vulkan_model_new (void);

gboolean
openvr_vulkan_model_load (OpenVRVulkanModel *self,
                          GulkanDevice             *device,
                          VkCommandBuffer           cmd_buffer,
                          const char               *model_name);

G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_MODEL_H_ */
