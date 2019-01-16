/*
 * Xrd GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_GLIB_SCENE_WINDOW_H_
#define XRD_GLIB_SCENE_WINDOW_H_

#include <glib-object.h>

#include "openvr-context.h"

#include <gulkan-vertex-buffer.h>
#include <gulkan-texture.h>
#include <gulkan-uniform-buffer.h>

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_WINDOW xrd_scene_window_get_type()
G_DECLARE_FINAL_TYPE (XrdSceneWindow, xrd_scene_window,
                      XRD, SCENE_WINDOW, GObject)

struct _XrdSceneWindow
{
  GObject parent;

  GulkanVertexBuffer *vertex_buffer;
  GulkanUniformBuffer *uniform_buffers[2];
  GulkanTexture *cat_texture;
  VkSampler scene_sampler;

  VkDescriptorPool descriptor_pool;
  VkDescriptorSet descriptor_sets[2];

  graphene_matrix_t model_matrix;

  graphene_point3d_t position;
  float scale;

  GulkanDevice *device;
};

XrdSceneWindow *xrd_scene_window_new (void);

bool
xrd_scene_window_init_texture (XrdSceneWindow *self,
                               GulkanDevice   *device,
                               VkCommandBuffer cmd_buffer,
                               GdkPixbuf      *pixbuf);

gboolean
xrd_scene_window_initialize (XrdSceneWindow        *self,
                             GulkanDevice          *device,
                             VkDescriptorSetLayout *layout);


void
xrd_scene_window_draw (XrdSceneWindow    *self,
                       EVREye             eye,
                       VkPipeline         pipeline,
                       VkPipelineLayout   pipeline_layout,
                       VkCommandBuffer    cmd_buffer,
                       graphene_matrix_t *vp);

void
xrd_scene_window_set_scale (XrdSceneWindow *self, float scale);

void
xrd_scene_window_set_position (XrdSceneWindow     *self,
                               graphene_point3d_t *position);

G_END_DECLS

#endif /* XRD_GLIB_SCENE_WINDOW_H_ */