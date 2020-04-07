/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-model.h"

#include <gxr.h>

#include "xrd-scene-renderer.h"

typedef struct {
  float mvp[16];
} XrdSceneModelUniformBuffer;

struct _XrdSceneModel
{
  XrdSceneObject parent;

  GulkanDevice *device;

  GulkanTexture *texture;
  GulkanVertexBuffer *vbo;
  VkSampler sampler;

  gchar *model_name;
};

static gchar *
_get_name (GxrModel *self)
{
  XrdSceneModel *scene_model = XRD_SCENE_MODEL (self);
  return scene_model->model_name;
}


static void
xrd_scene_model_interface_init (GxrModelInterface *iface)
{
  iface->get_name = _get_name;
}

G_DEFINE_TYPE_WITH_CODE (XrdSceneModel, xrd_scene_model, XRD_TYPE_SCENE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GXR_TYPE_MODEL,
                                                xrd_scene_model_interface_init))

static void
xrd_scene_model_finalize (GObject *gobject);

static void
xrd_scene_model_class_init (XrdSceneModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_model_finalize;
}

static void
xrd_scene_model_init (XrdSceneModel *self)
{
  self->sampler = VK_NULL_HANDLE;
  self->vbo = gulkan_vertex_buffer_new ();
}

XrdSceneModel *
xrd_scene_model_new (VkDescriptorSetLayout *layout)
{
  XrdSceneModel *self = (XrdSceneModel*) g_object_new (XRD_TYPE_SCENE_MODEL, 0);

  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);

  VkDeviceSize ub_size = sizeof (XrdSceneModelUniformBuffer);
  if (!xrd_scene_object_initialize (obj, layout, ub_size))
    return FALSE;

  return self;
}

static void
xrd_scene_model_finalize (GObject *gobject)
{
  XrdSceneModel *self = XRD_SCENE_MODEL (gobject);
  g_free (self->model_name);
  g_object_unref (self->vbo);
  g_object_unref (self->texture);

  XrdSceneRenderer *renderer = xrd_scene_renderer_get_instance ();
  GulkanClient *gc = xrd_scene_renderer_get_gulkan (renderer);
  GulkanDevice *device = gulkan_client_get_device (gc);

  if (self->sampler != VK_NULL_HANDLE)
    vkDestroySampler (gulkan_device_get_handle (device),
                      self->sampler, NULL);
}

VkSampler
xrd_scene_model_get_sampler (XrdSceneModel *self)
{
  return self->sampler;
}

GulkanVertexBuffer*
xrd_scene_model_get_vbo (XrdSceneModel *self)
{
  return self->vbo;
}

GulkanTexture*
xrd_scene_model_get_texture (XrdSceneModel *self)
{
  return self->texture;
}

gboolean
xrd_scene_model_load (XrdSceneModel *self,
                      GxrContext    *context,
                      const char    *model_name)
{
  gboolean res =
    gxr_context_load_model (context, self->vbo, &self->texture,
                            &self->sampler, model_name);

  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);

  xrd_scene_object_update_descriptors_texture (
    obj, self->sampler,
    gulkan_texture_get_image_view (self->texture));

  self->model_name = g_strdup (model_name);
  return res;
}

static void
_update_ubo (XrdSceneModel     *self,
             GxrEye             eye,
             graphene_matrix_t *transformation,
             graphene_matrix_t *vp)
{
  XrdSceneModelUniformBuffer ub;

  graphene_matrix_t mvp_matrix;
  graphene_matrix_multiply (transformation, vp, &mvp_matrix);
  graphene_matrix_to_float (&mvp_matrix, ub.mvp);

  xrd_scene_object_update_ubo (XRD_SCENE_OBJECT (self), eye, &ub);
}

void
xrd_scene_model_draw (XrdSceneModel     *self,
                      GxrEye             eye,
                      VkPipeline         pipeline,
                      VkCommandBuffer    cmd_buffer,
                      VkPipelineLayout   pipeline_layout,
                      graphene_matrix_t *transformation,
                      graphene_matrix_t *vp)
{
  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);
  if (!xrd_scene_object_is_visible (obj))
    return;

  vkCmdBindPipeline (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  _update_ubo (self, eye, transformation, vp);

  xrd_scene_object_bind (obj, eye, cmd_buffer, pipeline_layout);
  gulkan_vertex_buffer_draw_indexed (self->vbo, cmd_buffer);
}
