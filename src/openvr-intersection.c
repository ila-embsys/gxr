/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-intersection.h"
#include "openvr-math.h"

G_DEFINE_TYPE (OpenVRIntersection, openvr_intersection, OPENVR_TYPE_OVERLAY)

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
  (void) self;
}

GdkPixbuf *
_load_pixbuf (const gchar* name)
{
  GError * error = NULL;
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_resource (name, &error);

  if (error != NULL)
    {
      fprintf (stderr, "Unable to read file: %s\n", error->message);
      g_error_free (error);
      return NULL;
    }

  return pixbuf;
}

OpenVRIntersection *
openvr_intersection_new (const gchar* name)
{
  OpenVRIntersection *self =
    (OpenVRIntersection*) g_object_new (OPENVR_TYPE_INTERSECTION, 0);

  openvr_overlay_create_width (OPENVR_OVERLAY (self),
                               "intersection", "Intersection", 0.15);

  if (!openvr_overlay_is_valid (OPENVR_OVERLAY (self)))
    {
      g_printerr ("Intersection overlay unavailable.\n");
      return NULL;
    }

  GdkPixbuf *pixbuf = _load_pixbuf (name);
  if (pixbuf == NULL)
    return NULL;

  openvr_overlay_set_gdk_pixbuf_raw (OPENVR_OVERLAY (self), pixbuf);
  g_object_unref (pixbuf);

  /*
   * The crosshair should always be visible, except the pointer can
   * occlude it. The pointer has max sort order, so the crosshair gets max -1
   */
  openvr_overlay_set_sort_order (OPENVR_OVERLAY (self), UINT32_MAX - 1);

  return self;

}

static void
openvr_intersection_finalize (GObject *gobject)
{
  OpenVRIntersection *self = OPENVR_INTERSECTION (gobject);
  (void) self;
}

void
openvr_intersection_update (OpenVRIntersection *self,
                            graphene_matrix_t  *pose,
                            graphene_point3d_t *intersection_point)
{
  graphene_matrix_t transform;
  graphene_matrix_init_from_matrix (&transform, pose);
  openvr_math_matrix_set_translation (&transform, intersection_point);
  openvr_overlay_set_transform_absolute (OPENVR_OVERLAY (self), &transform);
  openvr_overlay_show (OPENVR_OVERLAY (self));
}
