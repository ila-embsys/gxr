/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef SCENE_BACKGROUND_H_
#define SCENE_BACKGROUND_H_

#include <glib-object.h>

#include <gxr.h>
#include <gulkan.h>

#include "scene-object.h"

G_BEGIN_DECLS

#define SCENE_TYPE_BACKGROUND scene_background_get_type()
G_DECLARE_FINAL_TYPE (SceneBackground, scene_background,
                      SCENE, BACKGROUND, SceneObject)

SceneBackground *
scene_background_new (GulkanClient          *gulkan,
                      VkDescriptorSetLayout *layout);

void
scene_background_render (SceneBackground    *self,
                         VkPipeline          pipeline,
                         VkPipelineLayout    pipeline_layout,
                         VkCommandBuffer     cmd_buffer,
                         graphene_matrix_t  *vp);

G_END_DECLS

#endif /* SCENE_BACKGROUND_H_ */
