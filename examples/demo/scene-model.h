/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef SCENE_MODEL_H_
#define SCENE_MODEL_H_

#include <glib-object.h>

#include <gulkan.h>
#include <gxr.h>

#include "scene-object.h"

G_BEGIN_DECLS

#define SCENE_TYPE_MODEL scene_model_get_type()
G_DECLARE_FINAL_TYPE (SceneModel, scene_model,
                      SCENE, MODEL, SceneObject)

SceneModel *
scene_model_new (VkDescriptorSetLayout *layout, GulkanClient *gulkan);

gboolean
scene_model_load (SceneModel *self,
                  GxrContext *context,
                  const char *model_name);

VkSampler
scene_model_get_sampler (SceneModel *self);

GulkanVertexBuffer*
scene_model_get_vbo (SceneModel *self);

GulkanTexture*
scene_model_get_texture (SceneModel *self);

void
scene_model_render (SceneModel        *self,
                    GxrEye             eye,
                    VkPipeline         pipeline,
                    VkCommandBuffer    cmd_buffer,
                    VkPipelineLayout   pipeline_layout,
                    graphene_matrix_t *transformation,
                    graphene_matrix_t *vp);

G_END_DECLS

#endif /* SCENE_MODEL_H_ */
