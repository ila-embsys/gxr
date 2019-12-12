/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <math.h>
#include <glib.h>
#include <GL/gl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

#include "gxr.h"

#define WIDTH 1280
#define HEIGHT 720
#define STRIDE (WIDTH * 4)

#include "cairo_content.h"

static GulkanTexture *texture;

static void
draw_cairo (cairo_t *cr, unsigned width, unsigned height)
{
  draw_gradient_quad (cr, width, height);
  draw_gradient_circle (cr, width, height);
}

static cairo_surface_t*
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

static gboolean
timeout_callback (gpointer data)
{
  OpenVROverlay *overlay = (OpenVROverlay*) data;
  openvr_overlay_poll_event (overlay);
  return TRUE;
}

static void
_press_cb (OpenVROverlay  *overlay,
           GdkEventButton *event,
           gpointer        data)
{
  (void) overlay;
  g_print ("press: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}

static void
_show_cb (OpenVROverlay *overlay,
          gpointer       data)
{
  g_print ("show\n");

  /* skip rendering if the overlay isn't available or visible */
  gboolean is_invisible = !openvr_overlay_is_visible (overlay) &&
                          !openvr_overlay_thumbnail_is_visible (overlay);

  if (!openvr_overlay_is_valid (overlay) || is_invisible)
    return;

  GulkanClient *uploader = (GulkanClient*) data;

  openvr_overlay_submit_texture (overlay, uploader, texture);
}

static void
_destroy_cb (OpenVROverlay *overlay,
             gpointer       data)
{
  (void) overlay;
  g_print ("destroy\n");
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}

static bool
_init_openvr (GxrContext *context, GulkanClient *client)
{
  if (!gxr_context_init_runtime (context, GXR_APP_OVERLAY))
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  if (!gxr_context_init_gulkan (context, client))
    {
      g_printerr ("Unable to initialize Vulkan!\n");
      return false;
    }

  if (!gxr_context_init_session (context, client))
    {
      g_printerr ("Could not init OpenVR session.\n");
      return false;
    }

  return true;
}

static int
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

  GxrContext *context = gxr_context_get_instance ();
  GulkanClient *uploader = openvr_compositor_gulkan_client_new ();

  /* init openvr */
  if (!_init_openvr (context, uploader))
    return -1;

  texture = gulkan_client_texture_new_from_cairo_surface (uploader, surface,
                                                          VK_FORMAT_R8G8B8A8_UNORM,
                                                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  /* create openvr overlay */
  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create (overlay, "examples.cairo", "Gradient");

  if (!openvr_overlay_is_valid (overlay))
  {
    fprintf (stderr, "Overlay unavailable.\n");
    return -1;
  }

  openvr_overlay_set_mouse_scale (overlay, (float) WIDTH, (float) HEIGHT);

  if (!openvr_overlay_show (overlay))
    return -1;

  openvr_overlay_submit_texture (overlay, uploader, texture);

  g_signal_connect (overlay, "button-press-event", (GCallback) _press_cb, loop);
  g_signal_connect (overlay, "show", (GCallback) _show_cb, uploader);
  g_signal_connect (overlay, "destroy", (GCallback) _destroy_cb, loop);

  g_timeout_add (20, timeout_callback, overlay);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_object_unref (overlay);
  cairo_surface_destroy (surface);
  g_object_unref (texture);
  g_object_unref (uploader);

  g_object_unref (context);

  return 0;
}

int main () {
  return test_cat_overlay ();
}
