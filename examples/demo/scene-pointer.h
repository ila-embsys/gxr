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

#define SCENE_TYPE_POINTER scene_pointer_get_type ()
G_DECLARE_FINAL_TYPE (ScenePointer, scene_pointer, SCENE, POINTER, SceneObject)

ScenePointer *
scene_pointer_new (GulkanContext *gulkan, GulkanDescriptorSet *descriptor_set);

void
scene_pointer_render (ScenePointer      *self,
                      GulkanPipeline    *pipeline,
                      VkPipelineLayout   pipeline_layout,
                      VkCommandBuffer    cmd_buffer,
                      graphene_matrix_t *vp);

float
scene_pointer_get_length (ScenePointer *self);

#endif /* SCENE_POINTER_H_ */
