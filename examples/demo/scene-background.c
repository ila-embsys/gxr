/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "scene-background.h"
#include <gulkan.h>
#include "graphene-ext.h"

#include <stdalign.h>

typedef struct {
  alignas(32) float mvp[2][16];
} SceneBackgroundUniformBuffer;

struct _SceneBackground
{
  SceneObject parent;
  GulkanVertexBuffer *vertex_buffer;
};

G_DEFINE_TYPE (SceneBackground, scene_background, SCENE_TYPE_OBJECT)

static void
scene_background_finalize (GObject *gobject);

static void
scene_background_class_init (SceneBackgroundClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = scene_background_finalize;
}

static void
scene_background_init (SceneBackground *self)
{
  self->vertex_buffer = NULL;
}

static gboolean
_initialize (SceneBackground       *self,
             GulkanContext          *gulkan,
             VkDescriptorSetLayout *layout);

SceneBackground *
scene_background_new (GulkanContext          *gulkan,
                      VkDescriptorSetLayout *layout)
{
  SceneBackground *self =
    (SceneBackground*) g_object_new (SCENE_TYPE_BACKGROUND, 0);

  _initialize (self, gulkan, layout);

  return self;
}

static void
scene_background_finalize (GObject *gobject)
{
  SceneBackground *self = SCENE_BACKGROUND (gobject);
  g_clear_object (&self->vertex_buffer);
  G_OBJECT_CLASS (scene_background_parent_class)->finalize (gobject);
}

static void
_append_star (GulkanVertexBuffer *self,
              float               radius,
              float               y,
              uint32_t            sections,
              graphene_vec3_t    *color)
{
  graphene_vec4_t *points = g_malloc (sizeof(graphene_vec4_t) * sections);

  graphene_vec4_init (&points[0],  radius, y, 0, 1);
  graphene_vec4_init (&points[1], -radius, y, 0, 1);

  graphene_matrix_t rotation;
  graphene_matrix_init_identity (&rotation);
  graphene_matrix_rotate_y (&rotation, 360.0f / (float) sections);

  for (uint32_t i = 0; i < sections / 2 - 1; i++)
    {
      uint32_t j = i * 2;
      graphene_matrix_transform_vec4 (&rotation, &points[j],     &points[j + 2]);
      graphene_matrix_transform_vec4 (&rotation, &points[j + 1], &points[j + 3]);
    }

  for (uint32_t i = 0; i < sections; i++)
    gulkan_vertex_buffer_append_with_color (self, &points[i], color);

  g_free (points);
}

static void
_append_circle (GulkanVertexBuffer *self,
                float               radius,
                float               y,
                uint32_t            edges,
                graphene_vec3_t    *color)
{
  graphene_vec4_t *points = g_malloc (sizeof(graphene_vec4_t) * edges * 2);

  graphene_vec4_init (&points[0], radius, y, 0, 1);

  graphene_matrix_t rotation;
  graphene_matrix_init_identity (&rotation);
  graphene_matrix_rotate_y (&rotation, 360.0f / (float) edges);

  for (uint32_t i = 0; i < edges; i++)
    {
      uint32_t j = i * 2;
      if (i != 0)
        graphene_vec4_init_from_vec4 (&points[j], &points[j - 1]);
      graphene_matrix_transform_vec4 (&rotation, &points[j], &points[j + 1]);
    }

  for (uint32_t i = 0; i < edges * 2; i++)
    gulkan_vertex_buffer_append_with_color (self, &points[i], color);

  g_free(points);
}

static void
_append_floor (GulkanVertexBuffer *self,
               uint32_t            radius,
               float               y,
               graphene_vec3_t    *color)
{
  _append_star (self, (float) radius, y, 8, color);

  for (uint32_t i = 1; i <= radius; i++)
    _append_circle (self, (float) i, y, 128, color);
}

static gboolean
_initialize (SceneBackground       *self,
             GulkanContext          *gulkan,
             VkDescriptorSetLayout *layout)
{
  GulkanDevice *device = gulkan_context_get_device (gulkan);
  self->vertex_buffer =
    gulkan_vertex_buffer_new (device, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

  gulkan_vertex_buffer_reset (self->vertex_buffer);

  graphene_vec3_t color;
  graphene_vec3_init (&color, .6f, .6f, .6f);

  _append_floor (self->vertex_buffer, 20, 0.0f, &color);
  _append_floor (self->vertex_buffer, 20, 4.0f, &color);

  if (!gulkan_vertex_buffer_alloc_empty (self->vertex_buffer,
                                         GXR_DEVICE_INDEX_MAX))
    return FALSE;

  gulkan_vertex_buffer_map_array (self->vertex_buffer);

  SceneObject *obj = SCENE_OBJECT (self);

  VkDeviceSize ub_size = sizeof (SceneBackgroundUniformBuffer);
  if (!scene_object_initialize (obj, gulkan, layout, ub_size))
    return FALSE;

  scene_object_update_descriptors (obj);

  return TRUE;
}

void
scene_background_render (SceneBackground    *self,
                         VkPipeline          pipeline,
                         VkPipelineLayout    pipeline_layout,
                         VkCommandBuffer     cmd_buffer,
                         graphene_matrix_t  *vp)
{
  if (!gulkan_vertex_buffer_is_initialized (self->vertex_buffer))
    return;

  SceneObject *obj = SCENE_OBJECT (self);
  if (!scene_object_is_visible (obj))
    return;

  vkCmdBindPipeline (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  SceneBackgroundUniformBuffer ub = {0};

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

  scene_object_bind (obj, cmd_buffer, pipeline_layout);
  gulkan_vertex_buffer_draw (self->vertex_buffer, cmd_buffer);
}
