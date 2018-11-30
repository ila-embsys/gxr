/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gdk/gdk.h>
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

#define WIDTH 512
#define HEIGHT 512

void
draw_gradient_circle (cairo_t *cr, unsigned width, unsigned height)
{
  double center_x = (double) width / 2.0;
  double center_y = (double) height / 2.0;

  double radius = center_x;

  cairo_pattern_t *pat = cairo_pattern_create_radial (center_x, center_y,
                                                      (3.0/4.0) * radius,
                                                      center_x, center_y,
                                                      radius);
  cairo_pattern_add_color_stop_rgba (pat, 0, 1, 1, 1, 1);
  cairo_pattern_add_color_stop_rgba (pat, 1, 1, 1, 1, 0);
  cairo_set_source (cr, pat);
  cairo_arc (cr, center_x, center_y, radius, 0, 2 * M_PI);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);
}

cairo_surface_t*
create_cairo_surface ()
{
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                         WIDTH, HEIGHT);

  cairo_t *cr = cairo_create (surface);
  cairo_set_source_rgba (cr, 0, 0, 0, 0);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  draw_gradient_circle (cr, WIDTH, HEIGHT);
  cairo_destroy (cr);

  return surface;
}

OpenVRIntersection *
openvr_intersection_new (int controller_index)
{
  OpenVRIntersection *self =
    (OpenVRIntersection*) g_object_new (OPENVR_TYPE_INTERSECTION, 0);

  char key[k_unVROverlayMaxKeyLength];
  snprintf (key, k_unVROverlayMaxKeyLength - 1, "intersection-%d",
            controller_index);

  openvr_overlay_create_width (OPENVR_OVERLAY (self),
                               key, key, 0.025);

  if (!openvr_overlay_is_valid (OPENVR_OVERLAY (self)))
    {
      g_printerr ("Intersection overlay unavailable.\n");
      return NULL;
    }

  cairo_surface_t* surface = create_cairo_surface ();
  GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0,
                                                   WIDTH, HEIGHT);
  cairo_surface_destroy (surface);

  /* Since we cannot set a different format for raw upload,
   * we need to use GdkPixbuf to suit OpenVRs needs */
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
