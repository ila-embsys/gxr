/*
 * Xrd GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-object.h"

G_DEFINE_TYPE (XrdSceneObject, xrd_scene_object, G_TYPE_OBJECT)

static void
xrd_scene_object_finalize (GObject *gobject);

static void
xrd_scene_object_class_init (XrdSceneObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_object_finalize;
}

static void
xrd_scene_object_init (XrdSceneObject *self)
{
  self->descriptor_pool = VK_NULL_HANDLE;
  graphene_matrix_init_identity (&self->model_matrix);
  self->scale = 1.0f;
  for (uint32_t eye = 0; eye < 2; eye++)
    self->uniform_buffers[eye] = gulkan_uniform_buffer_new ();
}

XrdSceneObject *
xrd_scene_object_new (void)
{
  return (XrdSceneObject*) g_object_new (XRD_TYPE_SCENE_OBJECT, 0);
}

static void
xrd_scene_object_finalize (GObject *gobject)
{
  XrdSceneObject *self = XRD_SCENE_OBJECT (gobject);
  vkDestroyDescriptorPool (self->device->device, self->descriptor_pool, NULL);

  for (uint32_t eye = 0; eye < 2; eye++)
    g_object_unref (self->uniform_buffers[eye]);
}

void
_update_model_matrix (XrdSceneObject *self)
{
  graphene_matrix_init_scale (&self->model_matrix,
                              self->scale, self->scale, self->scale);
  graphene_matrix_translate (&self->model_matrix, &self->position);
}

void
xrd_scene_object_update_mvp_matrix (XrdSceneObject    *self,
                                    EVREye             eye,
                                    graphene_matrix_t *vp)
{
 graphene_matrix_t mvp;
  graphene_matrix_multiply (&self->model_matrix, vp, &mvp);

  /* Update matrix in uniform buffer */
  graphene_matrix_to_float (&mvp, self->uniform_buffers[eye]->data);
}

void
xrd_scene_object_bind (XrdSceneObject    *self,
                       EVREye             eye,
                       VkCommandBuffer    cmd_buffer,
                       VkPipelineLayout   pipeline_layout)
{
  vkCmdBindDescriptorSets (
    cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
   &self->descriptor_sets[eye], 0, NULL);
}

void
xrd_scene_object_set_scale (XrdSceneObject *self, float scale)
{
  self->scale = scale;
  _update_model_matrix (self);
}

void
xrd_scene_object_set_position (XrdSceneObject     *self,
                               graphene_point3d_t *position)
{
  graphene_point3d_init_from_point (&self->position, position);
  _update_model_matrix (self);
}

