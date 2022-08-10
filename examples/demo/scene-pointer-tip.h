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

#define SCENE_TYPE_POINTER_TIP scene_pointer_tip_get_type()
G_DECLARE_FINAL_TYPE (ScenePointerTip, scene_pointer_tip,
                      SCENE, POINTER_TIP, SceneObject)

/*
 * Since the pulse animation surrounds the tip and would
 * exceed the canvas size, we need to scale it to fit the pulse.
 */
#define GXR_TIP_VIEWPORT_SCALE 3

/*
 * The distance in meters for which apparent size and regular size are equal.
 */
#define GXR_TIP_APPARENT_SIZE_DISTANCE 3.0f

ScenePointerTip *
scene_pointer_tip_new (GulkanContext          *gulkan,
                       VkDescriptorSetLayout *layout,
                       VkBuffer               lights);

void
scene_pointer_tip_render (ScenePointerTip    *self,
                          VkPipeline          pipeline,
                          VkPipelineLayout    pipeline_layout,
                          VkCommandBuffer     cmd_buffer,
                          graphene_matrix_t  *vp);

void
scene_pointer_tip_update_apparent_size (ScenePointerTip *self,
                                        GxrContext    *context);

void
scene_pointer_tip_update (ScenePointerTip      *self,
                          GxrContext         *context,
                          graphene_matrix_t  *pose,
                          graphene_point3d_t *intersection_point);

void
scene_pointer_tip_set_active (ScenePointerTip *self,
                              gboolean       active);

void
scene_pointer_tip_animate_pulse (ScenePointerTip *self);

void
scene_pointer_tip_set_width_meters (ScenePointerTip *self,
                                    float          meters);

void
scene_pointer_tip_set_and_submit_texture (ScenePointerTip *self,
                                          GulkanTexture *texture);

GulkanTexture *
scene_pointer_tip_get_texture (ScenePointerTip *self);

void
scene_pointer_tip_init_settings (ScenePointerTip *self);

GdkPixbuf*
scene_pointer_tip_update_pixbuf (ScenePointerTip *self,
                                 float          progress);

GulkanContext*
scene_pointer_tip_get_gulkan_context (ScenePointerTip *self);

void
scene_pointer_tip_update_texture_resolution (ScenePointerTip *self,
                                             int            width,
                                             int            height);

void
scene_pointer_tip_update_color (ScenePointerTip      *self,
                                gboolean            active_color,
                                graphene_point3d_t *color);

void
scene_pointer_tip_update_pulse_alpha (ScenePointerTip *self,
                                      double         alpha);

void
scene_pointer_tip_update_keep_apparent_size (ScenePointerTip *self,
                                             gboolean       keep_apparent_size);
void
scene_pointer_tip_update_width_meters (ScenePointerTip *self,
                                       float          width);

#endif /* SCENE_POINTER_TIP_H_ */
