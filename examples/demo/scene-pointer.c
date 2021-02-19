/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "scene-pointer.h"

#include <gulkan.h>

#include "graphene-ext.h"
#include "demo-pointer.h"

#include <stdalign.h>

typedef struct {
  alignas(16) float mvp[16];
} ScenePointerUniformBuffer;

static void
scene_pointer_interface_init (DemoPointerInterface *iface);

struct _ScenePointer
{
  SceneObject parent;
  GulkanVertexBuffer *vertex_buffer;

  DemoPointerData data;
};

G_DEFINE_TYPE_WITH_CODE (ScenePointer, scene_pointer, SCENE_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (DEMO_TYPE_POINTER,
                                                scene_pointer_interface_init))

static void
scene_pointer_finalize (GObject *gobject);

static void
scene_pointer_class_init (ScenePointerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = scene_pointer_finalize;
}

static void
scene_pointer_init (ScenePointer *self)
{
  self->vertex_buffer = gulkan_vertex_buffer_new ();

  demo_pointer_init (DEMO_POINTER (self));
}

static gboolean
_initialize (ScenePointer          *self,
             GulkanClient          *gulkan,
             VkDescriptorSetLayout *layout)
{
  gulkan_vertex_buffer_reset (self->vertex_buffer);

  graphene_vec4_t start;
  graphene_vec4_init (&start, 0, 0, self->data.start_offset, 1);

  graphene_matrix_t identity;
  graphene_matrix_init_identity (&identity);

  gulkan_geometry_append_ray (self->vertex_buffer,
                              &start, self->data.length, &identity);

  GulkanDevice *device = gulkan_client_get_device (gulkan);

  if (!gulkan_vertex_buffer_alloc_empty (self->vertex_buffer, device,
    GXR_DEVICE_INDEX_MAX))
    return FALSE;

  gulkan_vertex_buffer_map_array (self->vertex_buffer);

  SceneObject *obj = SCENE_OBJECT (self);

  VkDeviceSize ubo_size = sizeof (ScenePointerUniformBuffer);

  if (!scene_object_initialize (obj, gulkan, layout, ubo_size))
    return FALSE;

  scene_object_update_descriptors (obj);

  return TRUE;
}

ScenePointer *
scene_pointer_new (GulkanClient          *gulkan,
                   VkDescriptorSetLayout *layout)
{
  ScenePointer *self =
    (ScenePointer*) g_object_new (SCENE_TYPE_POINTER, 0);

  _initialize (self, gulkan, layout);
  return self;
}

static void
scene_pointer_finalize (GObject *gobject)
{
  ScenePointer *self = SCENE_POINTER (gobject);
  g_clear_object (&self->vertex_buffer);
  G_OBJECT_CLASS (scene_pointer_parent_class)->finalize (gobject);
}

static void
_update_ubo (ScenePointer      *self,
             GxrEye             eye,
             graphene_matrix_t *vp)
{
  ScenePointerUniformBuffer ub = {0};

  graphene_matrix_t m_matrix;
  scene_object_get_transformation (SCENE_OBJECT (self), &m_matrix);

  graphene_matrix_t mvp_matrix;
  graphene_matrix_multiply (&m_matrix, vp, &mvp_matrix);

  float mvp[16];
  graphene_matrix_to_float (&mvp_matrix, mvp);
  for (int i = 0; i < 16; i++)
    ub.mvp[i] = mvp[i];

  scene_object_update_ubo (SCENE_OBJECT (self), eye, &ub);
}

void
scene_pointer_render (ScenePointer      *self,
                      GxrEye             eye,
                      VkPipeline         pipeline,
                      VkPipelineLayout   pipeline_layout,
                      VkCommandBuffer    cmd_buffer,
                      graphene_matrix_t *vp)
{
  if (!gulkan_vertex_buffer_is_initialized (self->vertex_buffer))
    return;

  SceneObject *obj = SCENE_OBJECT (self);
  if (!scene_object_is_visible (obj))
    return;

  _update_ubo (self, eye, vp);

  vkCmdBindPipeline (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  scene_object_bind (obj, eye, cmd_buffer, pipeline_layout);
  gulkan_vertex_buffer_draw (self->vertex_buffer, cmd_buffer);
}

static void
_move (DemoPointer        *pointer,
       graphene_matrix_t *transform)
{
  ScenePointer *self = SCENE_POINTER (pointer);
  SceneObject *obj = SCENE_OBJECT (self);
  scene_object_set_transformation_direct (obj, transform);
}

static void
_set_length (DemoPointer *pointer,
             float       length)
{
  ScenePointer *self = SCENE_POINTER (pointer);
  gulkan_vertex_buffer_reset (self->vertex_buffer);

  graphene_matrix_t identity;
  graphene_matrix_init_identity (&identity);

  graphene_vec4_t start;
  graphene_vec4_init (&start, 0, 0, self->data.start_offset, 1);

  gulkan_geometry_append_ray (self->vertex_buffer, &start, length, &identity);
  gulkan_vertex_buffer_map_array (self->vertex_buffer);
}

static DemoPointerData*
_get_data (DemoPointer *pointer)
{
  ScenePointer *self = SCENE_POINTER (pointer);
  return &self->data;
}

static void
_set_transformation (DemoPointer        *pointer,
                     graphene_matrix_t *matrix)
{
  ScenePointer *self = SCENE_POINTER (pointer);
  scene_object_set_transformation (SCENE_OBJECT (self), matrix);
}

static void
_get_transformation (DemoPointer        *pointer,
                     graphene_matrix_t *matrix)
{
  ScenePointer *self = SCENE_POINTER (pointer);
  graphene_matrix_t transformation;
  scene_object_get_transformation (SCENE_OBJECT (self), &transformation);
  graphene_matrix_init_from_matrix (matrix, &transformation);
}

static void
_show (DemoPointer *pointer)
{
  ScenePointer *self = SCENE_POINTER (pointer);
  scene_object_show (SCENE_OBJECT (self));
}

static void
_hide (DemoPointer *pointer)
{
  ScenePointer *self = SCENE_POINTER (pointer);
  scene_object_hide (SCENE_OBJECT (self));
}

static void
scene_pointer_interface_init (DemoPointerInterface *iface)
{
  iface->move = _move;
  iface->set_length = _set_length;
  iface->get_data = _get_data;
  iface->set_transformation = _set_transformation;
  iface->get_transformation = _get_transformation;
  iface->show = _show;
  iface->hide = _hide;
}

