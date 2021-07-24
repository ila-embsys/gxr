/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "scene-object.h"
#include <gulkan.h>

#include "scene-renderer.h"
#include "graphene-ext.h"

typedef struct _SceneObjectPrivate
{
  GObject parent;

  GulkanClient *gulkan;

  GulkanUniformBuffer *uniform_buffer;

  GulkanDescriptorPool *descriptor_pool;
  VkDescriptorSet descriptor_set;

  graphene_matrix_t model_matrix;

  graphene_point3d_t position;
  float scale;
  graphene_quaternion_t orientation;

  gboolean visible;

  gboolean initialized;
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

  priv->descriptor_pool = NULL;
  graphene_matrix_init_identity (&priv->model_matrix);
  priv->scale = 1.0f;
  priv->visible = TRUE;
  priv->initialized = FALSE;
  priv->gulkan = NULL;
}

static void
scene_object_finalize (GObject *gobject)
{
  SceneObject *self = SCENE_OBJECT (gobject);
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  if (!priv->initialized)
    return;

  g_object_unref (priv->descriptor_pool);
  g_object_unref (priv->uniform_buffer);

  g_clear_object (&priv->gulkan);

  G_OBJECT_CLASS (scene_object_parent_class)->finalize (gobject);
}

static void
_update_model_matrix (SceneObject *self)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  graphene_matrix_init_scale (&priv->model_matrix,
                              priv->scale, priv->scale, priv->scale);
  graphene_matrix_rotate_quaternion (&priv->model_matrix, &priv->orientation);
  graphene_matrix_translate (&priv->model_matrix, &priv->position);
}

void
scene_object_bind (SceneObject     *self,
                   VkCommandBuffer  cmd_buffer,
                   VkPipelineLayout pipeline_layout)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  vkCmdBindDescriptorSets (
    cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
   &priv->descriptor_set, 0, NULL);
}

void
scene_object_set_scale (SceneObject *self, float scale)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  priv->scale = scale;
  _update_model_matrix (self);
}

void
scene_object_set_position (SceneObject        *self,
                           graphene_point3d_t *position)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  graphene_point3d_init_from_point (&priv->position, position);
  _update_model_matrix (self);
}

void
scene_object_get_position (SceneObject        *self,
                           graphene_point3d_t *position)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  graphene_point3d_init_from_point (position, &priv->position);
}

void
scene_object_set_rotation_euler (SceneObject      *self,
                                 graphene_euler_t *euler)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  graphene_quaternion_init_from_euler (&priv->orientation, euler);
  _update_model_matrix (self);
}

gboolean
scene_object_initialize (SceneObject           *self,
                         GulkanClient          *gulkan,
                         VkDescriptorSetLayout *layout,
                         VkDeviceSize           uniform_buffer_size)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  priv->gulkan = g_object_ref (gulkan);

  GulkanDevice *device = gulkan_client_get_device (gulkan);
  VkDevice vk_device = gulkan_device_get_handle (device);

  /* Create uniform buffer to hold a matrix per eye */
  priv->uniform_buffer =
    gulkan_uniform_buffer_new (device, uniform_buffer_size);
  if (!priv->uniform_buffer)
    return FALSE;

  uint32_t set_count = 2;

  VkDescriptorPoolSize pool_sizes[] = {
    {
      .descriptorCount = set_count,
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    },
    {
      .descriptorCount = set_count,
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    },
    {
      .descriptorCount = set_count,
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    },
    {
      .descriptorCount = set_count,
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    }
  };

  priv->descriptor_pool =
    gulkan_descriptor_pool_new_from_layout (vk_device, *layout, pool_sizes,
                                            G_N_ELEMENTS (pool_sizes),
                                            set_count);
  if (!priv->descriptor_pool)
    return FALSE;

  if (!gulkan_descriptor_pool_allocate_sets (priv->descriptor_pool,
                                             1, &priv->descriptor_set))
    return FALSE;

  priv->initialized = TRUE;

  return TRUE;
}

void
scene_object_update_descriptors_texture (SceneObject *self,
                                         VkSampler    sampler,
                                         VkImageView  image_view)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  VkDevice device = gulkan_client_get_device_handle (priv->gulkan);

  VkWriteDescriptorSet *write_descriptor_sets = (VkWriteDescriptorSet []) {
    {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = priv->descriptor_set,
      .dstBinding = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .pBufferInfo = &(VkDescriptorBufferInfo) {
        .buffer = gulkan_uniform_buffer_get_handle (priv->uniform_buffer),
        .offset = 0,
        .range = VK_WHOLE_SIZE
      },
      .pTexelBufferView = NULL
    },
    {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = priv->descriptor_set,
      .dstBinding = 1,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &(VkDescriptorImageInfo) {
        .sampler = sampler,
        .imageView = image_view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      },
      .pBufferInfo = NULL,
      .pTexelBufferView = NULL
    }
  };

  vkUpdateDescriptorSets (device, 2, write_descriptor_sets, 0, NULL);
}

void
scene_object_update_descriptors (SceneObject *self)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  VkDevice device = gulkan_client_get_device_handle (priv->gulkan);

  VkWriteDescriptorSet *write_descriptor_sets = (VkWriteDescriptorSet []) {
    {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = priv->descriptor_set,
      .dstBinding = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .pBufferInfo = &(VkDescriptorBufferInfo) {
        .buffer = gulkan_uniform_buffer_get_handle (priv->uniform_buffer),
        .offset = 0,
        .range = VK_WHOLE_SIZE
      },
      .pTexelBufferView = NULL
    }
  };

  vkUpdateDescriptorSets (device, 1, write_descriptor_sets, 0, NULL);
}

void
scene_object_set_transformation (SceneObject       *self,
                                 graphene_matrix_t *mat)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  graphene_ext_matrix_get_rotation_quaternion (mat, &priv->orientation);
  graphene_ext_matrix_get_translation_point3d (mat, &priv->position);

  // graphene_vec3_t scale;
  // graphene_matrix_get_scale (mat, &scale);

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

graphene_matrix_t
scene_object_get_transformation_no_scale (SceneObject *self)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);

  graphene_matrix_t mat;
  graphene_matrix_init_identity (&mat);
  graphene_matrix_rotate_quaternion (&mat, &priv->orientation);
  graphene_matrix_translate (&mat, &priv->position);
  return mat;
}

gboolean
scene_object_is_visible (SceneObject *self)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  return priv->visible;
}

void
scene_object_show (SceneObject *self)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  priv->visible = TRUE;
}

void
scene_object_hide (SceneObject *self)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  priv->visible = FALSE;
}

GulkanUniformBuffer *
scene_object_get_ubo (SceneObject *self)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  return priv->uniform_buffer;
}

VkBuffer
scene_object_get_transformation_buffer (SceneObject *self)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  return gulkan_uniform_buffer_get_handle (priv->uniform_buffer);
}

VkDescriptorSet
scene_object_get_descriptor_set (SceneObject *self)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  return priv->descriptor_set;
}

void
scene_object_update_ubo (SceneObject *self,
                         gpointer uniform_buffer)
{
  SceneObjectPrivate *priv = scene_object_get_instance_private (self);
  gulkan_uniform_buffer_update (priv->uniform_buffer, uniform_buffer);
}
