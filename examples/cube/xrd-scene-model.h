/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_SCENE_MODEL_H_
#define XRD_SCENE_MODEL_H_

#include <glib-object.h>

#include <gulkan.h>
#include <gxr.h>

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_MODEL xrd_scene_model_get_type()
G_DECLARE_FINAL_TYPE (XrdSceneModel, xrd_scene_model,
                      XRD, SCENE_MODEL, GObject)

XrdSceneModel *xrd_scene_model_new (void);

gboolean
xrd_scene_model_load (XrdSceneModel *self,
                      GxrContext    *context,
                      const char    *model_name);

VkSampler
xrd_scene_model_get_sampler (XrdSceneModel *self);

GulkanVertexBuffer*
xrd_scene_model_get_vbo (XrdSceneModel *self);

GulkanTexture*
xrd_scene_model_get_texture (XrdSceneModel *self);

G_END_DECLS

#endif /* XRD_SCENE_MODEL_H_ */
