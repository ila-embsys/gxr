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
  self->active = FALSE;
  self->active_texture = NULL;
  self->default_texture = NULL;
}

#define WIDTH 64
#define HEIGHT 64

void
draw_gradient_circle (cairo_t *cr,
                      double r,
                      double g,
                      double b)
{
  double center_x = (double) WIDTH / 2.0;
  double center_y = (double) HEIGHT / 2.0;

  double radius = center_x;

  cairo_pattern_t *pat = cairo_pattern_create_radial (center_x, center_y,
                                                      0.75 * radius,
                                                      center_x, center_y,
                                                      radius);
  cairo_pattern_add_color_stop_rgba (pat, 0, r, g, b, 1);

  cairo_pattern_add_color_stop_rgba (pat, 1, r, g, b, 0);
  cairo_set_source (cr, pat);
  cairo_arc (cr, center_x, center_y, radius, 0, 2 * M_PI);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);
}

GdkPixbuf*
_render_tip_pixbuf (double r, double g, double b)
{
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                         WIDTH, HEIGHT);

  cairo_t *cr = cairo_create (surface);
  cairo_set_source_rgba (cr, 0, 0, 0, 0);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  draw_gradient_circle (cr, r, g, b);

  cairo_destroy (cr);

  /* Since we cannot set a different format for raw upload,
   * we need to use GdkPixbuf to suit OpenVRs needs */
  GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0,
                                                   WIDTH, HEIGHT);

  cairo_surface_destroy (surface);

  return pixbuf;
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

  /*
   * The crosshair should always be visible, except the pointer can
   * occlude it. The pointer has max sort order, so the crosshair gets max -1
   */
  openvr_overlay_set_sort_order (OPENVR_OVERLAY (self), UINT32_MAX - 1);

  return self;
}

void
openvr_intersection_init_vulkan (OpenVRIntersection   *self,
                                 OpenVRVulkanUploader *uploader)
{
  GulkanClient *client = GULKAN_CLIENT (uploader);

  GdkPixbuf* default_pixbuf = _render_tip_pixbuf (1.0, 1.0, 1.0);
  self->default_texture = gulkan_texture_new_from_pixbuf (client->device,
                                                          default_pixbuf);
  gulkan_client_upload_pixbuf (client, self->default_texture, default_pixbuf);
  g_object_unref (default_pixbuf);

  GdkPixbuf* active_pixbuf = _render_tip_pixbuf (0.078, 0.471, 0.675);
  self->active_texture = gulkan_texture_new_from_pixbuf (client->device,
                                                         active_pixbuf);
  gulkan_client_upload_pixbuf (client, self->active_texture, active_pixbuf);
  g_object_unref (active_pixbuf);
}

void
openvr_intersection_init_raw (OpenVRIntersection *self)
{
  GdkPixbuf *default_pixbuf = _render_tip_pixbuf (1.0, 1.0, 1.0);
  openvr_overlay_set_gdk_pixbuf_raw (OPENVR_OVERLAY (self), default_pixbuf);
  g_object_unref (default_pixbuf);
}

void
openvr_intersection_set_active (OpenVRIntersection *self,
                                OpenVRVulkanUploader *uploader,
                                gboolean active)
{
  if (self->active_texture == NULL)
    return;

  if (active != self->active)
    {
      if (active)
        openvr_vulkan_uploader_submit_frame (uploader, OPENVR_OVERLAY (self),
                                             self->active_texture);
      else
        openvr_vulkan_uploader_submit_frame (uploader, OPENVR_OVERLAY (self),
                                             self->default_texture);
      self->active = active;
    }

}

static void
openvr_intersection_finalize (GObject *gobject)
{
  OpenVRIntersection *self = OPENVR_INTERSECTION (gobject);
  (void) self;
  if (self->active_texture)
    g_object_unref (self->active_texture);
  if (self->default_texture)
    g_object_unref (self->default_texture);
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
}
