/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef SCENE_OBJECT_H_
#define SCENE_OBJECT_H_

#include "glib.h"
#include <gulkan.h>
#include <graphene.h>
#include <gxr.h>

G_BEGIN_DECLS

#define SCENE_TYPE_OBJECT scene_object_get_type()
G_DECLARE_DERIVABLE_TYPE (SceneObject, scene_object,
                          SCENE, OBJECT, GObject)

/**
 * SceneObjectClass:
 * @parent: The object class structure needs to be the first
 *   element in the widget class structure in order for the class mechanism
 *   to work correctly. This allows a SceneObjectClass pointer to be cast to
 *   a GObjectClass pointer.
 */
struct _SceneObjectClass
{
  GObjectClass parent;
};

void
scene_object_set_scale (SceneObject *self, float scale);

void
scene_object_set_position (SceneObject        *self,
                           graphene_point3d_t *position);

void
scene_object_get_position (SceneObject        *self,
                           graphene_point3d_t *position);

void
scene_object_set_rotation_euler (SceneObject      *self,
                                 graphene_euler_t *euler);

void
scene_object_get_transformation (SceneObject       *self,
                                 graphene_matrix_t *transformation);

void
scene_object_bind (SceneObject     *self,
                   VkCommandBuffer  cmd_buffer,
                   VkPipelineLayout pipeline_layout);

gboolean
scene_object_initialize (SceneObject           *self,
                         GulkanContext          *gulkan,
                         VkDescriptorSetLayout *layout,
                         VkDeviceSize           uniform_buffer_size);

void
scene_object_update_descriptors_texture (SceneObject *self,
                                         VkSampler    sampler,
                                         VkImageView  image_view);

void
scene_object_update_descriptors (SceneObject *self);

void
scene_object_set_transformation (SceneObject       *self,
                                 graphene_matrix_t *mat);

graphene_matrix_t
scene_object_get_transformation_no_scale (SceneObject *self);

gboolean
scene_object_is_visible (SceneObject *self);
void
scene_object_show (SceneObject *self);

void
scene_object_hide (SceneObject *self);

void
scene_object_set_transformation_direct (SceneObject       *self,
                                        graphene_matrix_t *mat);

GulkanUniformBuffer *
scene_object_get_ubo (SceneObject *self);

VkBuffer
scene_object_get_transformation_buffer (SceneObject *self);

VkDescriptorSet
scene_object_get_descriptor_set (SceneObject *self);

void
scene_object_update_ubo (SceneObject *self,
                         gpointer     uniform_buffer);

G_END_DECLS

#endif /* SCENE_OBJECT_H_ */
