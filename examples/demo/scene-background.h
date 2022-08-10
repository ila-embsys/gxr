/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef SCENE_BACKGROUND_H_
#define SCENE_BACKGROUND_H_

#include <glib-object.h>

#include <gulkan.h>
#include <gxr.h>

#include "scene-object.h"

#define SCENE_TYPE_BACKGROUND scene_background_get_type ()
G_DECLARE_FINAL_TYPE (SceneBackground,
                      scene_background,
                      SCENE,
                      BACKGROUND,
                      SceneObject)

SceneBackground *
scene_background_new (GulkanContext *gulkan, VkDescriptorSetLayout *layout);

void
scene_background_render (SceneBackground   *self,
                         VkPipeline         pipeline,
                         VkPipelineLayout   pipeline_layout,
                         VkCommandBuffer    cmd_buffer,
                         graphene_matrix_t *vp);

#endif /* SCENE_BACKGROUND_H_ */
