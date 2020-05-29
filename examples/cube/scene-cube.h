/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef SCENE_CUBE_H_
#define SCENE_CUBE_H_

#include <glib-object.h>

#include <gulkan.h>

#include "scene-object.h"

G_BEGIN_DECLS

#define SCENE_TYPE_CUBE scene_cube_get_type()
G_DECLARE_FINAL_TYPE (SceneCube, scene_cube,
                      SCENE, CUBE, SceneObject)

SceneCube *
scene_cube_new (GulkanClient         *gulkan,
                GulkanRenderer       *renderer,
                GulkanRenderPass     *render_pass,
                VkSampleCountFlagBits sample_count);

void
scene_cube_render (SceneCube          *self,
                   GxrEye              eye,
                   VkCommandBuffer     cmd_buffer,
                   graphene_matrix_t  *view,
                   graphene_matrix_t  *projection);

G_END_DECLS

#endif /* SCENE_CUBE_H_ */
