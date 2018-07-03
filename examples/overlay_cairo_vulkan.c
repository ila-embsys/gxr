/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <math.h>
#include <glib.h>
#include <GL/gl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

#include <openvr-glib.h>

#include "openvr-system.h"
#include "openvr-overlay.h"
#include "openvr-compositor.h"
#include "openvr-vulkan-uploader.h"

#define WIDTH 1280
#define HEIGHT 720
#define STRIDE (WIDTH * 4)

#include "cairo_content.h"

OpenVRVulkanTexture *texture;

void
draw_cairo (cairo_t *cr, unsigned width, unsigned height)
{
  draw_gradient_quad (cr, width, height);
  draw_gradient_circle (cr, width, height);
}

cairo_surface_t*
create_cairo_surface (unsigned char *image)
{
  cairo_surface_t *surface =
    cairo_image_surface_create_for_data (image,
                                         CAIRO_FORMAT_ARGB32,
                                         WIDTH, HEIGHT,
                                         STRIDE);

  cairo_t *cr = cairo_create (surface);

  cairo_rectangle (cr, 0, 0, WIDTH, HEIGHT);
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_fill (cr);

  draw_cairo (cr, WIDTH, HEIGHT);

  cairo_destroy (cr);

  return surface;
}

gboolean
timeout_callback (gpointer data)
{
  OpenVROverlay *overlay = (OpenVROverlay*) data;
  openvr_overlay_poll_event (overlay);
  return TRUE;
}

static void
_move_cb (OpenVROverlay  *overlay,
          GdkEventMotion *event,
          gpointer        data)
{
  // g_print ("move: %f %f (%d)\n", event->x, event->y, event->time);
}

static void
_press_cb (OpenVROverlay  *overlay,
           GdkEventButton *event,
           gpointer        data)
{
  g_print ("press: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}

static void
_release_cb (OpenVROverlay  *overlay,
             GdkEventButton *event,
             gpointer        data)
{
  g_print ("release: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
}

static void
_show_cb (OpenVROverlay *overlay,
          gpointer       data)
{
  g_print ("show\n");

  /* skip rendering if the overlay isn't available or visible */
  gboolean is_unavailable = !openvr_overlay_is_valid (overlay) ||
                            !openvr_overlay_is_available (overlay);
  gboolean is_invisible = !openvr_overlay_is_visible (overlay) &&
                          !openvr_overlay_thumbnail_is_visible (overlay);

  if (is_unavailable || is_invisible)
    return;

  OpenVRVulkanUploader * uploader = (OpenVRVulkanUploader*) data;

  openvr_vulkan_uploader_submit_frame (uploader, overlay, texture);
}

static void
_destroy_cb (OpenVROverlay *overlay,
             gpointer       data)
{
  g_print ("destroy\n");
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}

int
test_cat_overlay ()
{
  GMainLoop *loop;

  unsigned char image[STRIDE*HEIGHT];
  cairo_surface_t* surface = create_cairo_surface (image);

  if (surface == NULL) {
    fprintf (stderr, "Could not create cairo surface.\n");
    return -1;
  }

  loop = g_main_loop_new (NULL, FALSE);

  /* init openvr */
  OpenVRSystem * system = openvr_system_new ();
  gboolean ret = openvr_system_init_overlay (system);
  OpenVRCompositor *compositor = openvr_compositor_new ();

  g_assert (ret);
  g_assert (openvr_system_is_available (system));
  g_assert (openvr_system_is_installed ());

  /* Upload vulkan texture */
  OpenVRVulkanUploader *uploader = openvr_vulkan_uploader_new ();
  if (!openvr_vulkan_uploader_init_vulkan (uploader, false, system, compositor))
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return false;
  }

  texture = openvr_vulkan_texture_new ();

  openvr_vulkan_uploader_load_cairo_surface (uploader, texture, surface);

  /* create openvr overlay */
  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create_for_dashboard (overlay, "examples.cairo", "Gradient");

  if (!openvr_overlay_is_valid (overlay) ||
      !openvr_overlay_is_available (overlay))
  {
    fprintf (stderr, "Overlay unavailable.\n");
    return -1;
  }

  openvr_overlay_set_mouse_scale (overlay, (float) WIDTH, (float) HEIGHT);

  g_signal_connect (overlay, "motion-notify-event", (GCallback) _move_cb, NULL);
  g_signal_connect (overlay, "button-press-event", (GCallback) _press_cb, loop);
  g_signal_connect (
    overlay, "button-release-event", (GCallback) _release_cb, NULL);
  g_signal_connect (overlay, "show", (GCallback) _show_cb, uploader);
  g_signal_connect (overlay, "destroy", (GCallback) _destroy_cb, loop);

  g_timeout_add (20, timeout_callback, overlay);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_object_unref (overlay);
  cairo_surface_destroy (surface);
  g_object_unref (texture);
  g_object_unref (uploader);

  return 0;
}

int main (int argc, char *argv[]) {
  return test_cat_overlay ();
}
