/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-model.h"

#include <gxr.h>

#include "xrd-scene-renderer.h"

struct _XrdSceneModel
{
  GObject parent;

  GulkanDevice *device;

  GulkanTexture *texture;
  GulkanVertexBuffer *vbo;
  VkSampler sampler;
};

G_DEFINE_TYPE (XrdSceneModel, xrd_scene_model, G_TYPE_OBJECT)

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
xrd_scene_model_new (void)
{
  return (XrdSceneModel*) g_object_new (XRD_TYPE_SCENE_MODEL, 0);
}

static void
xrd_scene_model_finalize (GObject *gobject)
{
  XrdSceneModel *self = XRD_SCENE_MODEL (gobject);
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
  return gxr_context_load_model (context, self->vbo, &self->texture,
                                &self->sampler, model_name);
}

