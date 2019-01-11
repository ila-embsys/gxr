/*
 * Xrd GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_GLIB_SCENE_POINTER_H_
#define XRD_GLIB_SCENE_POINTER_H_

#include <glib-object.h>

#include <gulkan-vertex-buffer.h>

#include "openvr-context.h"

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_POINTER xrd_scene_pointer_get_type()
G_DECLARE_FINAL_TYPE (XrdScenePointer, xrd_scene_pointer,
                      XRD, SCENE_POINTER, GObject)

struct _XrdScenePointer
{
  GObject parent;

  GulkanVertexBuffer *pointer_vbo;
};

XrdScenePointer *xrd_scene_pointer_new (void);

void
xrd_scene_pointer_render_pointer (XrdScenePointer *self,
                                  VkPipeline       pipeline,
                                  VkCommandBuffer  cmd_buffer);

#define MAX_TRACKED_DEVICES 64

void
xrd_scene_pointer_update (XrdScenePointer    *self,
                          GulkanDevice       *device,
                          TrackedDevicePose_t device_poses[MAX_TRACKED_DEVICES],
                          graphene_matrix_t   device_mats[MAX_TRACKED_DEVICES]);

G_END_DECLS

#endif /* XRD_GLIB_SCENE_POINTER_H_ */
