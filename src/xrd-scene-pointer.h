/*
 * Xrd GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_GLIB_SCENE_POINTER_H_
#define XRD_GLIB_SCENE_POINTER_H_

#include <glib-object.h>

#include <gulkan-vertex-buffer.h>

#include "openvr-context.h"
#include <gulkan-uniform-buffer.h>

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_POINTER xrd_scene_pointer_get_type()
G_DECLARE_FINAL_TYPE (XrdScenePointer, xrd_scene_pointer,
                      XRD, SCENE_POINTER, GObject)

struct _XrdScenePointer
{
  GObject parent;

  VkDescriptorPool descriptor_pool;
  VkDescriptorSet descriptor_sets[2];
  GulkanUniformBuffer *uniform_buffers[2];

  GulkanDevice *device;

  graphene_matrix_t model_matrix;

  GulkanVertexBuffer *vertex_buffer;
};

XrdScenePointer *xrd_scene_pointer_new (void);

gboolean
xrd_scene_pointer_initialize (XrdScenePointer       *self,
                              GulkanDevice          *device,
                              VkDescriptorSetLayout *layout);

void
xrd_scene_pointer_render (XrdScenePointer   *self,
                          EVREye             eye,
                          VkPipeline         pipeline,
                          VkPipelineLayout   pipeline_layout,
                          VkCommandBuffer    cmd_buffer,
                          graphene_matrix_t *vp);

void
xrd_scene_pointer_update (XrdScenePointer    *self,
                          graphene_vec4_t    *start,
                          float               length);

G_END_DECLS

#endif /* XRD_GLIB_SCENE_POINTER_H_ */
