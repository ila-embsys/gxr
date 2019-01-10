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

/* the backing texture behind the intersection overlay will have
 * WIDTH*2 x HEIGHT*2 pixels to keep room for the pulse animation */
#define WIDTH 64
#define HEIGHT 64

#define ACTIVE_R 0.078
#define ACTIVE_G 0.471
#define ACTIVE_B 0.675

#define BACKGROUND_ALPHA 0.1

typedef struct Animation
{
  OpenVRIntersection *self;
  OpenVRVulkanUploader *uploader;
  float progress;
} Animation;

/** draws a circle in the center of a cairo surface of dimensions WIDTHxHEIGHT.
 * scale affects the radius of the circle and should be in [0,2].
 * a_in is the alpha value at the center, a_out at the outer border. */
void
draw_gradient_circle (cairo_t *cr,
                      double r,
                      double g,
                      double b,
                      double a_in,
                      double a_out,
                      float  scale)
{
  double center_x = WIDTH;
  double center_y = HEIGHT;

  double radius = (WIDTH / 2.) * scale;

  cairo_pattern_t *pat = cairo_pattern_create_radial (center_x, center_y,
                                                      0.75 * radius,
                                                      center_x, center_y,
                                                      radius);
  cairo_pattern_add_color_stop_rgba (pat, 0, r, g, b, a_in);

  cairo_pattern_add_color_stop_rgba (pat, 1, r, g, b, a_out);
  cairo_set_source (cr, pat);
  cairo_arc (cr, center_x, center_y, radius, 0, 2 * M_PI);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);
}

GdkPixbuf*
_render_tip_pixbuf (double r, double g, double b, float background_scale)
{
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                         WIDTH*2, HEIGHT*2);

  cairo_t *cr = cairo_create (surface);
  cairo_set_source_rgba (cr, 0, 0, 0, 0);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  draw_gradient_circle (cr, 1.0, 1.0, 1.0, 0.1, 0.1, background_scale);

  cairo_set_operator (cr, CAIRO_OPERATOR_MULTIPLY);
  draw_gradient_circle (cr, r, g, b, 1.0, 0.0, 1.0);

  cairo_destroy (cr);

  /* Since we cannot set a different format for raw upload,
   * we need to use GdkPixbuf to suit OpenVRs needs */
  GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0,
                                                   WIDTH * 2, HEIGHT * 2);

  cairo_surface_destroy (surface);

  return pixbuf;
}

gboolean
_animate_cb (gpointer _animation)
{
  Animation *animation = (Animation *) _animation;

  GulkanClient *client = GULKAN_CLIENT (animation->uploader);
  GdkPixbuf* active_pixbuf =_render_tip_pixbuf (ACTIVE_R,
                                                ACTIVE_G,
                                                ACTIVE_B,
                                                2 - 2 * animation->progress);
  gulkan_client_upload_pixbuf (client,
                               animation->self->active_texture,
                               active_pixbuf);
  g_object_unref (active_pixbuf);

  openvr_vulkan_uploader_submit_frame (animation->uploader,
                                       OPENVR_OVERLAY (animation->self),
                                       animation->self->active_texture);

  animation->progress += 0.05f;

  if (animation->progress > 1)
    {
      g_free (animation);
      return FALSE;
    }

  return TRUE;
}

void
openvr_intersection_animate_pulse (OpenVRIntersection *self,
                                   OpenVRVulkanUploader *uploader)
{
  Animation *animation =  g_malloc (sizeof *animation);
  animation->progress = 0;
  animation->uploader = uploader;
  animation->self = self;
  g_timeout_add (20, _animate_cb, animation);
}

OpenVRIntersection *
openvr_intersection_new (int controller_index)
{
  OpenVRIntersection *self =
    (OpenVRIntersection*) g_object_new (OPENVR_TYPE_INTERSECTION, 0);

  char key[k_unVROverlayMaxKeyLength];
  snprintf (key, k_unVROverlayMaxKeyLength - 1, "intersection-%d",
            controller_index);

  /* the texture has 2x the size of the pointer, so the overlay should be 2x
   * the desired size of the default pointer too. */
  openvr_overlay_create_width (OPENVR_OVERLAY (self),
                               key, key, 0.025 * 2);

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

  GdkPixbuf* default_pixbuf = _render_tip_pixbuf (1.0, 1.0, 1.0, 0.0);
  self->default_texture =
    gulkan_texture_new_from_pixbuf (client->device, default_pixbuf,
                                    VK_FORMAT_R8G8B8A8_UNORM);
  gulkan_client_upload_pixbuf (client, self->default_texture, default_pixbuf);
  g_object_unref (default_pixbuf);

  GdkPixbuf* active_pixbuf = _render_tip_pixbuf (ACTIVE_R, ACTIVE_G,
                                                 ACTIVE_B, 0.0);
  self->active_texture =
    gulkan_texture_new_from_pixbuf (client->device, active_pixbuf,
                                    VK_FORMAT_R8G8B8A8_UNORM);
  gulkan_client_upload_pixbuf (client, self->active_texture, active_pixbuf);
  g_object_unref (active_pixbuf);
}

void
openvr_intersection_init_raw (OpenVRIntersection *self)
{
  GdkPixbuf *default_pixbuf = _render_tip_pixbuf (1.0, 1.0, 1.0, 0.0);
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
