/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-intersection.h"

G_DEFINE_TYPE (OpenVRIntersection, openvr_intersection, G_TYPE_OBJECT)

static void
openvr_intersection_finalize (GObject *gobject);

static void
openvr_intersection_class_init (OpenVRIntersectionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_intersection_finalize;
}

static void
openvr_intersection_init (OpenVRIntersection *self)
{
  self->index = 1337;
}

OpenVRIntersection *
openvr_intersection_new (void)
{
  return (OpenVRIntersection*) g_object_new (OPENVR_TYPE_INTERSECTION, 0);
}

static void
openvr_intersection_finalize (GObject *gobject)
{
  OpenVRIntersection *self = OPENVR_INTERSECTION (gobject);
  (void) self;
}
