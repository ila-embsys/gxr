/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_INTERSECTION_H_
#define OPENVR_GLIB_INTERSECTION_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_INTERSECTION openvr_intersection_get_type()
G_DECLARE_FINAL_TYPE (OpenVRIntersection, openvr_intersection, OPENVR, INTERSECTION, GObject)

struct _OpenVRIntersection
{
  GObject parent;

  guint index;
};

OpenVRIntersection *openvr_intersection_new (void);

G_END_DECLS

#endif /* OPENVR_GLIB_INTERSECTION_H_ */
