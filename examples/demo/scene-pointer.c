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

#include <stdalign.h>

typedef struct
{
  alignas (32) float mvp[2][16];
} ScenePointerUniformBuffer;

struct _ScenePointer
{
  SceneObject         parent;
  GulkanVertexBuffer *vertex_buffer;

  float start_offset;
  float length;
};

G_DEFINE_TYPE (ScenePointer, scene_pointer, SCENE_TYPE_OBJECT)

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
  self->vertex_buffer = NULL;
  self->start_offset = -0.02f;
  self->length = 5.0f;
}

static gboolean
_initialize (ScenePointer        *self,
             GulkanContext       *gulkan,
             GulkanDescriptorSet *descriptor_set)
{
  GulkanDevice *device = gulkan_context_get_device (gulkan);
  self->vertex_buffer
    = gulkan_vertex_buffer_new (device, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
  gulkan_vertex_buffer_reset (self->vertex_buffer);

  graphene_vec4_t start;
  graphene_vec4_init (&start, 0, 0, self->start_offset, 1);

  graphene_matrix_t identity;
  graphene_matrix_init_identity (&identity);

  gulkan_geometry_append_ray (self->vertex_buffer, &start, self->length,
                              &identity);

  if (!gulkan_vertex_buffer_alloc_empty (self->vertex_buffer,
                                         GXR_DEVICE_INDEX_MAX))
    return FALSE;

  gulkan_vertex_buffer_map_array (self->vertex_buffer);

  SceneObject *obj = SCENE_OBJECT (self);

  VkDeviceSize ubo_size = sizeof (ScenePointerUniformBuffer);

  if (!scene_object_initialize (obj, gulkan, ubo_size, descriptor_set))
    return FALSE;

  scene_object_update_descriptors (obj);

  return TRUE;
}

ScenePointer *
scene_pointer_new (GulkanContext *gulkan, GulkanDescriptorSet *descriptor_set)
{
  ScenePointer *self = (ScenePointer *) g_object_new (SCENE_TYPE_POINTER, 0);

  _initialize (self, gulkan, descriptor_set);
  return self;
}

static void
scene_pointer_finalize (GObject *gobject)
{
  ScenePointer *self = SCENE_POINTER (gobject);
  g_clear_object (&self->vertex_buffer);
  G_OBJECT_CLASS (scene_pointer_parent_class)->finalize (gobject);
}

void
scene_pointer_render (ScenePointer      *self,
                      GulkanPipeline    *pipeline,
                      VkPipelineLayout   pipeline_layout,
                      VkCommandBuffer    cmd_buffer,
                      graphene_matrix_t *vp)
{
  if (!gulkan_vertex_buffer_is_initialized (self->vertex_buffer))
    return;

  SceneObject *obj = SCENE_OBJECT (self);

  ScenePointerUniformBuffer ub = {0};

  graphene_matrix_t m_matrix;
  scene_object_get_transformation (SCENE_OBJECT (self), &m_matrix);

  for (uint32_t eye = 0; eye < 2; eye++)
    {
      graphene_matrix_t mvp_matrix;
      graphene_matrix_multiply (&m_matrix, &vp[eye], &mvp_matrix);

      float mvp[16];
      graphene_matrix_to_float (&mvp_matrix, mvp);
      for (int i = 0; i < 16; i++)
        ub.mvp[eye][i] = mvp[i];
    }

  scene_object_update_ubo (SCENE_OBJECT (self), &ub);

  gulkan_pipeline_bind (pipeline, cmd_buffer);

  scene_object_bind (obj, cmd_buffer, pipeline_layout);
  gulkan_vertex_buffer_draw (self->vertex_buffer, cmd_buffer);
}

float
scene_pointer_get_length (ScenePointer *self)
{
  return self->length;
}
