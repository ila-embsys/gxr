/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "scene-model.h"

#include <gxr.h>

#include <stdalign.h>

typedef struct {
  alignas(16) float mvp[16];
} SceneModelUniformBuffer;

struct _SceneModel
{
  SceneObject parent;

  GulkanClient *gulkan;

  GulkanTexture *texture;
  GulkanVertexBuffer *vbo;
  VkSampler sampler;

  gchar *model_name;
};

static gchar *
_get_name (GxrModel *self)
{
  SceneModel *scene_model = SCENE_MODEL (self);
  return scene_model->model_name;
}


static void
scene_model_interface_init (GxrModelInterface *iface)
{
  iface->get_name = _get_name;
}

G_DEFINE_TYPE_WITH_CODE (SceneModel, scene_model, SCENE_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GXR_TYPE_MODEL,
                                                scene_model_interface_init))

static void
scene_model_finalize (GObject *gobject);

static void
scene_model_class_init (SceneModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = scene_model_finalize;
}

static void
scene_model_init (SceneModel *self)
{
  self->sampler = VK_NULL_HANDLE;
  self->vbo = gulkan_vertex_buffer_new ();
}

SceneModel *
scene_model_new (VkDescriptorSetLayout *layout, GulkanClient *gulkan)
{
  SceneModel *self = (SceneModel*) g_object_new (SCENE_TYPE_MODEL, 0);

  self->gulkan = g_object_ref (gulkan);

  SceneObject *obj = SCENE_OBJECT (self);

  VkDeviceSize ub_size = sizeof (SceneModelUniformBuffer);
  if (!scene_object_initialize (obj, gulkan, layout, ub_size))
    return FALSE;

  return self;
}

static void
scene_model_finalize (GObject *gobject)
{
  SceneModel *self = SCENE_MODEL (gobject);
  g_free (self->model_name);
  g_clear_object (&self->vbo);
  g_clear_object (&self->texture);

  GulkanDevice *device = gulkan_client_get_device (self->gulkan);

  if (self->sampler != VK_NULL_HANDLE)
    vkDestroySampler (gulkan_device_get_handle (device),
                      self->sampler, NULL);

  g_clear_object (&self->gulkan);

  G_OBJECT_CLASS (scene_model_parent_class)->finalize (gobject);
}

VkSampler
scene_model_get_sampler (SceneModel *self)
{
  return self->sampler;
}

GulkanVertexBuffer*
scene_model_get_vbo (SceneModel *self)
{
  return self->vbo;
}

GulkanTexture*
scene_model_get_texture (SceneModel *self)
{
  return self->texture;
}

gboolean
scene_model_load (SceneModel *self,
                  GxrContext *context,
                  const char *model_name)
{
  gboolean res =
    gxr_context_load_model (context, self->vbo, &self->texture,
                            &self->sampler, model_name);

  SceneObject *obj = SCENE_OBJECT (self);

  scene_object_update_descriptors_texture (
    obj, self->sampler,
    gulkan_texture_get_image_view (self->texture));

  self->model_name = g_strdup (model_name);
  return res;
}

static void
_update_ubo (SceneModel        *self,
             GxrEye             eye,
             graphene_matrix_t *transformation,
             graphene_matrix_t *vp)
{
  SceneModelUniformBuffer ub = {0};

  graphene_matrix_t mvp_matrix;
  graphene_matrix_multiply (transformation, vp, &mvp_matrix);
  graphene_matrix_to_float (&mvp_matrix, ub.mvp);

  scene_object_update_ubo (SCENE_OBJECT (self), eye, &ub);
}

void
scene_model_render (SceneModel            *self,
                        GxrEye             eye,
                        VkPipeline         pipeline,
                        VkCommandBuffer    cmd_buffer,
                        VkPipelineLayout   pipeline_layout,
                        graphene_matrix_t *transformation,
                        graphene_matrix_t *vp)
{
  SceneObject *obj = SCENE_OBJECT (self);
  if (!scene_object_is_visible (obj))
    return;

  vkCmdBindPipeline (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  _update_ubo (self, eye, transformation, vp);

  scene_object_bind (obj, eye, cmd_buffer, pipeline_layout);
  gulkan_vertex_buffer_draw_indexed (self->vbo, cmd_buffer);
}
