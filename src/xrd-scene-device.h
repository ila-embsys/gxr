/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_VULKAN_MODEL_H_
#define OPENVR_GLIB_VULKAN_MODEL_H_

#include <glib-object.h>


#include <graphene.h>
#include <openvr_capi.h>

#include <gulkan-uniform-buffer.h>

#include "openvr-vulkan-model.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_VULKAN_MODEL openvr_vulkan_model_get_type()
G_DECLARE_FINAL_TYPE (OpenVRVulkanModel, openvr_vulkan_model,
                      OPENVR, VULKAN_MODEL, GObject)

struct _OpenVRVulkanModel
{
  GObject parent;

  GulkanDevice *device;

  OpenVRVulkanModelContent *content;

  GulkanUniformBuffer *ubos[2];

  VkDescriptorPool descriptor_pool;
  VkDescriptorSet descriptor_sets[2];
};

OpenVRVulkanModel *openvr_vulkan_model_new (void);

gboolean
openvr_vulkan_model_initialize (OpenVRVulkanModel        *self,
                                OpenVRVulkanModelContent *content,
                                GulkanDevice             *device,
                                VkDescriptorSetLayout    *layout);

void
openvr_vulkan_model_draw (OpenVRVulkanModel *self,
                          EVREye             eye,
                          VkCommandBuffer    cmd_buffer,
                          VkPipelineLayout   pipeline_layout,
                          graphene_matrix_t *mvp);

G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_MODEL_H_ */
