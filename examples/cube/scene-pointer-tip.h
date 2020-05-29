/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef SCENE_POINTER_TIP_H_
#define SCENE_POINTER_TIP_H_

#include <glib-object.h>
#include "gulkan.h"
#include "gxr.h"
#include "scene-object.h"

G_BEGIN_DECLS

#define SCENE_TYPE_POINTER_TIP scene_pointer_tip_get_type()
G_DECLARE_FINAL_TYPE (ScenePointerTip, scene_pointer_tip,
                      SCENE, POINTER_TIP, SceneObject)

ScenePointerTip *
scene_pointer_tip_new (GulkanClient          *gulkan,
                       VkDescriptorSetLayout *layout,
                       VkBuffer               lights);

void
scene_pointer_tip_render (ScenePointerTip   *self,
                          GxrEye             eye,
                          VkPipeline         pipeline,
                          VkPipelineLayout   pipeline_layout,
                          VkCommandBuffer    cmd_buffer,
                          graphene_matrix_t *vp);

G_END_DECLS

#endif /* SCENE_POINTER_TIP_H_ */
