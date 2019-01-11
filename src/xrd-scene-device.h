/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_SCENE_DEVICE_H_
#define XRD_SCENE_DEVICE_H_

#include <glib-object.h>


#include <graphene.h>
#include <openvr_capi.h>

#include <gulkan-uniform-buffer.h>

#include "openvr-vulkan-model.h"

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_DEVICE xrd_scene_device_get_type()
G_DECLARE_FINAL_TYPE (XrdSceneDevice, xrd_scene_device,
                      XRD, SCENE_DEVICE, GObject)

struct _XrdSceneDevice
{
  GObject parent;

  GulkanDevice *device;

  OpenVRVulkanModel *content;

  GulkanUniformBuffer *ubos[2];

  VkDescriptorPool descriptor_pool;
  VkDescriptorSet descriptor_sets[2];
};

XrdSceneDevice *xrd_scene_device_new (void);

gboolean
xrd_scene_device_initialize (XrdSceneDevice        *self,
                             OpenVRVulkanModel     *content,
                             GulkanDevice          *device,
                             VkDescriptorSetLayout *layout);

void
xrd_scene_device_draw (XrdSceneDevice    *self,
                       EVREye             eye,
                       VkCommandBuffer    cmd_buffer,
                       VkPipelineLayout   pipeline_layout,
                       graphene_matrix_t *mvp);

G_END_DECLS

#endif /* XRD_SCENE_DEVICE_H_ */
