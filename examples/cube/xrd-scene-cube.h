/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_SCENE_CUBE_H_
#define XRD_SCENE_CUBE_H_

#include <glib-object.h>

#include <gulkan.h>

#include "xrd-scene-object.h"

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_CUBE xrd_scene_cube_get_type()
G_DECLARE_FINAL_TYPE (XrdSceneCube, xrd_scene_cube,
                      XRD, SCENE_CUBE, XrdSceneObject)

XrdSceneCube *xrd_scene_cube_new (void);

gboolean
xrd_scene_cube_initialize (XrdSceneCube *self,
                           GxrContext *context,
                           GulkanRenderer *renderer,
                           GulkanRenderPass *render_pass);

void
xrd_scene_cube_render (XrdSceneCube       *self,
                       GxrEye              eye,
                       VkCommandBuffer     cmd_buffer,
                       graphene_matrix_t  *view,
                       graphene_matrix_t  *projection);

G_END_DECLS

#endif /* XRD_SCENE_CUBE_H_ */
