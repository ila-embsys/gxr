/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_INTERSECTION_H_
#define OPENVR_GLIB_INTERSECTION_H_

#include <glib-object.h>
#include "openvr-overlay.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_INTERSECTION openvr_intersection_get_type()
G_DECLARE_FINAL_TYPE (OpenVRIntersection, openvr_intersection, OPENVR,
                      INTERSECTION, OpenVROverlay)

struct _OpenVRIntersection
{
  OpenVROverlay parent;
  GdkPixbuf* default_pixbuf;
  GdkPixbuf* active_pixbuf;
  gboolean active;
};

OpenVRIntersection *openvr_intersection_new (int controller_index);

void
openvr_intersection_update (OpenVRIntersection *self,
                            graphene_matrix_t  *pose,
                            graphene_point3d_t *intersection_point);

void
openvr_intersection_set_active (OpenVRIntersection *self, gboolean active);

G_END_DECLS

#endif /* OPENVR_GLIB_INTERSECTION_H_ */
