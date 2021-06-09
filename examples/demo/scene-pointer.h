/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef SCENE_POINTER_H_
#define SCENE_POINTER_H_

#include <glib-object.h>

#include <gulkan.h>
#include <gxr.h>

#include "scene-object.h"

G_BEGIN_DECLS

#define SCENE_TYPE_POINTER scene_pointer_get_type()
G_DECLARE_FINAL_TYPE (ScenePointer, scene_pointer,
                      SCENE, POINTER, SceneObject)

ScenePointer *
scene_pointer_new (GulkanClient          *gulkan,
                   VkDescriptorSetLayout *layout);

void
scene_pointer_render (ScenePointer      *self,
                      GxrEye             eye,
                      VkPipeline         pipeline,
                      VkPipelineLayout   pipeline_layout,
                      VkCommandBuffer    cmd_buffer,
                      graphene_matrix_t *vp);

void
scene_pointer_move (ScenePointer *self,
                    graphene_matrix_t *transform);

void
scene_pointer_set_length (ScenePointer *self,
                          float       length);

float
scene_pointer_get_default_length (ScenePointer *self);

void
scene_pointer_reset_length (ScenePointer *self);

void
scene_pointer_get_ray (ScenePointer     *self,
                       graphene_ray_t *res);

gboolean
scene_pointer_get_plane_intersection (ScenePointer        *self,
                                      graphene_plane_t  *plane,
                                      graphene_matrix_t *plane_transform,
                                      float              plane_aspect,
                                      float             *distance,
                                      graphene_vec3_t   *res);

void
scene_pointer_show (ScenePointer *self);

void
scene_pointer_hide (ScenePointer *self);

gboolean
scene_pointer_is_visible (ScenePointer *self);

void
scene_pointer_update_render_ray (ScenePointer *self, gboolean render_ray);

G_END_DECLS

#endif /* SCENE_POINTER_H_ */
