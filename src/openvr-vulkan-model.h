/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_VULKAN_MODEL_CONTENT_H_
#define OPENVR_GLIB_VULKAN_MODEL_CONTENT_H_

#include <glib-object.h>

#include <gulkan-texture.h>
#include "gulkan-vertex-buffer.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_VULKAN_MODEL_CONTENT openvr_vulkan_model_content_get_type()
G_DECLARE_FINAL_TYPE (OpenVRVulkanModelContent, openvr_vulkan_model_content,
                      OPENVR, VULKAN_MODEL_CONTENT, GObject)

struct _OpenVRVulkanModelContent
{
  GObject parent;

  GulkanDevice *device;

  GulkanTexture *texture;
  GulkanVertexBuffer *vbo;
  VkSampler sampler;
};

OpenVRVulkanModelContent *openvr_vulkan_model_content_new (void);

gboolean
openvr_vulkan_model_content_load (OpenVRVulkanModelContent *self,
                                  GulkanDevice             *device,
                                  VkCommandBuffer           cmd_buffer,
                                  const char               *model_name);

G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_MODEL_CONTENT_H_ */
