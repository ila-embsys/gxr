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

typedef struct {
  alignas(16) float mvp[16];
} ScenePointerUniformBuffer;

struct _ScenePointer
{
  SceneObject parent;
  GulkanVertexBuffer *vertex_buffer;

  float start_offset;
  float length;
  float default_length;
  gboolean visible;
  gboolean render_ray;
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
  self->default_length = 5.0f;
  self->length = self->default_length;
  self->visible = TRUE;
}

static gboolean
_initialize (ScenePointer          *self,
             GulkanClient          *gulkan,
             VkDescriptorSetLayout *layout)
{
  GulkanDevice *device = gulkan_client_get_device (gulkan);
  self->vertex_buffer =
    gulkan_vertex_buffer_new (device, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
  gulkan_vertex_buffer_reset (self->vertex_buffer);

  graphene_vec4_t start;
  graphene_vec4_init (&start, 0, 0, self->start_offset, 1);

  graphene_matrix_t identity;
  graphene_matrix_init_identity (&identity);

  gulkan_geometry_append_ray (self->vertex_buffer,
                              &start, self->length, &identity);

  if (!gulkan_vertex_buffer_alloc_empty (self->vertex_buffer,
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

void
scene_pointer_move (ScenePointer        *self,
                    graphene_matrix_t *transform)
{
  scene_object_set_transformation_direct (SCENE_OBJECT (self), transform);
}

void
scene_pointer_set_length (ScenePointer *self,
                          float       length)
{
  if (length == self->length)
    return;

  self->length = length;

  gulkan_vertex_buffer_reset (self->vertex_buffer);

  graphene_matrix_t identity;
  graphene_matrix_init_identity (&identity);

  graphene_vec4_t start;
  graphene_vec4_init (&start, 0, 0, self->start_offset, 1);

  gulkan_geometry_append_ray (self->vertex_buffer, &start, length, &identity);
  gulkan_vertex_buffer_map_array (self->vertex_buffer);
}

float
scene_pointer_get_default_length (ScenePointer *self)
{
  return self->default_length;
}

void
scene_pointer_reset_length (ScenePointer *self)
{
  scene_pointer_set_length (self, self->default_length);
}

void
scene_pointer_get_ray (ScenePointer     *self,
                       graphene_ray_t *res)
{
  graphene_matrix_t mat;
  scene_object_get_transformation (SCENE_OBJECT (self), &mat);

  graphene_vec4_t start;
  graphene_vec4_init (&start, 0, 0, self->start_offset, 1);
  graphene_matrix_transform_vec4 (&mat, &start, &start);

  graphene_vec4_t end;
  graphene_vec4_init (&end, 0, 0, -self->length, 1);
  graphene_matrix_transform_vec4 (&mat, &end, &end);

  graphene_vec4_t direction_vec4;
  graphene_vec4_subtract (&end, &start, &direction_vec4);

  graphene_point3d_t origin;
  graphene_vec3_t direction;

  graphene_vec3_t vec3_start;
  graphene_vec4_get_xyz (&start, &vec3_start);
  graphene_point3d_init_from_vec3 (&origin, &vec3_start);

  graphene_vec4_get_xyz (&direction_vec4, &direction);

  graphene_ray_init (res, &origin, &direction);
}

gboolean
scene_pointer_get_plane_intersection (ScenePointer        *self,
                                      graphene_plane_t  *plane,
                                      graphene_matrix_t *plane_transform,
                                      float              plane_aspect,
                                      float             *distance,
                                      graphene_vec3_t   *res)
{
  graphene_ray_t ray;
  scene_pointer_get_ray (self, &ray);

  *distance = graphene_ray_get_distance_to_plane (&ray, plane);
  if (*distance == INFINITY)
    return FALSE;

  graphene_ray_get_direction (&ray, res);
  graphene_vec3_scale (res, *distance, res);

  graphene_vec3_t origin;
  graphene_ext_ray_get_origin_vec3 (&ray, &origin);
  graphene_vec3_add (&origin, res, res);

  graphene_matrix_t inverse_plane_transform;

  graphene_matrix_inverse (plane_transform, &inverse_plane_transform);

  graphene_vec4_t intersection_vec4;
  graphene_vec4_init_from_vec3 (&intersection_vec4, res, 1.0f);

  graphene_vec4_t intersection_origin;
  graphene_matrix_transform_vec4 (&inverse_plane_transform,
                                  &intersection_vec4,
                                  &intersection_origin);

  float f[4];
  graphene_vec4_to_float (&intersection_origin, f);

  if (f[0] >= -plane_aspect / 2.0f && f[0] <= plane_aspect / 2.0f
      && f[1] >= -0.5f && f[1] <= 0.5f)
    return TRUE;

  return FALSE;
}

void
scene_pointer_show (ScenePointer *self)
{
  if (!self->render_ray)
    {
      return;
    }

  scene_object_show (SCENE_OBJECT (self));
  self->visible = TRUE;
}

void
scene_pointer_hide (ScenePointer *self)
{
  scene_object_hide (SCENE_OBJECT (self));
  self->visible = FALSE;
}

gboolean
scene_pointer_is_visible (ScenePointer *self)
{
  if (!self->render_ray)
    return FALSE;

  return self->visible;
}

void
scene_pointer_update_render_ray (ScenePointer *self, gboolean render_ray)
{
  self->render_ray = render_ray;

  if (!self->visible && render_ray)
    scene_pointer_show (self);
  else if (self->visible && !render_ray)
    scene_pointer_hide (self);
}
