/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "scene-object.h"
#include <gulkan.h>

#include "graphene-ext.h"

typedef struct _SceneObjectPrivate
{
  GObject parent;

  GulkanContext *gulkan;

  GulkanUniformBuffer *uniform_buffer;

  GulkanDescriptorSet *descriptor_set;

  graphene_matrix_t model_matrix;

  graphene_point3d_t    position;
  float                 scale;
  graphene_quaternion_t orientation;
} SceneObjectPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (SceneObject, scene_object, G_TYPE_OBJECT)

static void
scene_object_finalize (GObject *gobject);

static void
scene_object_class_init (SceneObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = scene_object_finalize;
}

static void
scene_object_init (SceneObject *self)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);

  graphene_matrix_init_identity (&priv->model_matrix);
  priv->scale = 1.0f;
  priv->gulkan = NULL;
}

static void
scene_object_finalize (GObject *gobject)
{
  SceneObject        *self = SCENE_OBJECT (gobject);
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);

  g_object_unref (priv->uniform_buffer);

  g_clear_object (&priv->gulkan);

  G_OBJECT_CLASS (scene_object_parent_class)->finalize (gobject);
}

static void
_update_model_matrix (SceneObject *self)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  graphene_matrix_init_scale (&priv->model_matrix, priv->scale, priv->scale,
                              priv->scale);
  graphene_matrix_rotate_quaternion (&priv->model_matrix, &priv->orientation);
  graphene_matrix_translate (&priv->model_matrix, &priv->position);
}

void
scene_object_bind (SceneObject     *self,
                   VkCommandBuffer  cmd_buffer,
                   VkPipelineLayout layout)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  gulkan_descriptor_set_bind (priv->descriptor_set, layout, cmd_buffer);
}

gboolean
scene_object_initialize (SceneObject         *self,
                         GulkanContext       *gulkan,
                         VkDeviceSize         uniform_buffer_size,
                         GulkanDescriptorSet *descriptor_set)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  priv->gulkan = g_object_ref (gulkan);

  GulkanDevice *device = gulkan_context_get_device (gulkan);

  /* Create uniform buffer to hold a matrix per eye */
  priv->uniform_buffer = gulkan_uniform_buffer_new (device,
                                                    uniform_buffer_size);
  if (!priv->uniform_buffer)
    return FALSE;

  priv->descriptor_set = descriptor_set;

  return TRUE;
}

void
scene_object_update_descriptors (SceneObject *self)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  gulkan_descriptor_set_update_buffer (priv->descriptor_set, 0,
                                       priv->uniform_buffer);
}

void
scene_object_set_transformation (SceneObject *self, graphene_matrix_t *mat)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  graphene_point3d_t  unused_scale;
  graphene_ext_matrix_get_rotation_quaternion (mat, &unused_scale,
                                               &priv->orientation);
  graphene_ext_matrix_get_translation_point3d (mat, &priv->position);

  _update_model_matrix (self);
}

/*
 * Set transformation without matrix decomposition and ability to rebuild
 * This will include scale as well.
 */

void
scene_object_set_transformation_direct (SceneObject       *self,
                                        graphene_matrix_t *mat)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  graphene_matrix_init_from_matrix (&priv->model_matrix, mat);
}

void
scene_object_get_transformation (SceneObject       *self,
                                 graphene_matrix_t *transformation)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  graphene_matrix_init_from_matrix (transformation, &priv->model_matrix);
}

void
scene_object_update_ubo (SceneObject *self, gpointer uniform_buffer)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  gulkan_uniform_buffer_update (priv->uniform_buffer, uniform_buffer);
}
