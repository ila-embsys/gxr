/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-device.h"

#include <gxr.h>
#include <gulkan.h>

typedef struct {
  float mvp[16];
} XrdSceneDeviceUniformBuffer;

struct _XrdSceneDevice
{
  XrdSceneObject parent;

  XrdSceneModel *model;

  gboolean pose_valid;
  gboolean is_controller;
};

G_DEFINE_TYPE (XrdSceneDevice, xrd_scene_device, XRD_TYPE_SCENE_OBJECT)

static void
xrd_scene_device_finalize (GObject *gobject);

static void
xrd_scene_device_class_init (XrdSceneDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_device_finalize;
}

static void
xrd_scene_device_init (XrdSceneDevice *self)
{
  self->model = NULL;
  self->pose_valid = FALSE;
  self->is_controller = FALSE;
}

XrdSceneDevice *
xrd_scene_device_new (void)
{
  return (XrdSceneDevice*) g_object_new (XRD_TYPE_SCENE_DEVICE, 0);
}

static void
xrd_scene_device_finalize (GObject *gobject)
{
  XrdSceneDevice *self = XRD_SCENE_DEVICE (gobject);
  g_object_unref (self->model);
  G_OBJECT_CLASS (xrd_scene_device_parent_class)->finalize (gobject);
}

gboolean
xrd_scene_device_initialize (XrdSceneDevice        *self,
                             XrdSceneModel         *model,
                             VkDescriptorSetLayout *layout)
{
  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);

  VkDeviceSize ub_size = sizeof (XrdSceneDeviceUniformBuffer);
  if (!xrd_scene_object_initialize (obj, layout, ub_size))
    return FALSE;

  self->model = model;
  g_object_ref (self->model);

  xrd_scene_object_update_descriptors_texture (
    obj, xrd_scene_model_get_sampler (self->model),
    gulkan_texture_get_image_view (xrd_scene_model_get_texture (self->model)));

  return TRUE;
}

void
xrd_scene_device_set_is_controller (XrdSceneDevice *self, bool is_controller)
{
  self->is_controller = is_controller;
}

gboolean
xrd_scene_device_is_controller (XrdSceneDevice *self)
{
  return self->is_controller;
}

void
xrd_scene_device_set_is_pose_valid (XrdSceneDevice *self, bool valid)
{
  self->pose_valid = valid;
}

static void
_update_ubo (XrdSceneDevice    *self,
             GxrEye             eye,
             graphene_matrix_t *vp)
{
  XrdSceneDeviceUniformBuffer ub;

  graphene_matrix_t m_matrix;
  xrd_scene_object_get_transformation (XRD_SCENE_OBJECT (self), &m_matrix);

  graphene_matrix_t mvp_matrix;
  graphene_matrix_multiply (&m_matrix, vp, &mvp_matrix);
  graphene_matrix_to_float (&mvp_matrix, ub.mvp);

  xrd_scene_object_update_ubo (XRD_SCENE_OBJECT (self), eye, &ub);
}

void
xrd_scene_device_draw (XrdSceneDevice    *self,
                       GxrEye             eye,
                       VkCommandBuffer    cmd_buffer,
                       VkPipelineLayout   pipeline_layout,
                       graphene_matrix_t *vp)
{
  if (!self->pose_valid)
    return;

  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);
  if (!xrd_scene_object_is_visible (obj))
    return;

  _update_ubo (self, eye, vp);

  xrd_scene_object_bind (obj, eye, cmd_buffer, pipeline_layout);
  gulkan_vertex_buffer_draw_indexed (xrd_scene_model_get_vbo (self->model),
                                     cmd_buffer);
}

