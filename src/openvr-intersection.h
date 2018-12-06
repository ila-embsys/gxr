/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_INTERSECTION_H_
#define OPENVR_GLIB_INTERSECTION_H_

#include <glib-object.h>
#include <gulkan-texture.h>

#include "openvr-vulkan-uploader.h"
#include "openvr-overlay.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_INTERSECTION openvr_intersection_get_type()
G_DECLARE_FINAL_TYPE (OpenVRIntersection, openvr_intersection, OPENVR,
                      INTERSECTION, OpenVROverlay)

struct _OpenVRIntersection
{
  OpenVROverlay parent;
  GulkanTexture *default_texture;
  GulkanTexture *active_texture;
  gboolean active;
};

OpenVRIntersection *openvr_intersection_new (int controller_index);

void
openvr_intersection_update (OpenVRIntersection *self,
                            graphene_matrix_t  *pose,
                            graphene_point3d_t *intersection_point);

void
openvr_intersection_set_active (OpenVRIntersection *self,
                                OpenVRVulkanUploader *uploader,
                                gboolean active);

void
openvr_intersection_init_vulkan (OpenVRIntersection   *self,
                                 OpenVRVulkanUploader *uploader);

void
openvr_intersection_init_raw (OpenVRIntersection *self);

G_END_DECLS

#endif /* OPENVR_GLIB_INTERSECTION_H_ */
