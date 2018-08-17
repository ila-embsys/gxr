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
#include <glib/gprintf.h>

#include <openvr-glib.h>

#include "openvr-context.h"
#include "openvr-overlay.h"
#include "openvr-compositor.h"
#include "openvr-vulkan-uploader.h"
#include "openvr-time.h"

#define WIDTH 1000
#define HEIGHT 1000
#define STRIDE (WIDTH * 4)

#include "cairo_content.h"

struct timespec last_time;
unsigned frames_without_time_update = 60;
gchar fps_str [50];

void
update_fps (struct timespec* now)
{
  struct timespec diff;
  openvr_time_substract (now, &last_time, &diff);
  double diff_s = openvr_time_to_double_secs (&diff);
  double diff_ms = diff_s * SEC_IN_MSEC_D;
  double fps = SEC_IN_MSEC_D / diff_ms;
  g_sprintf (fps_str, "FPS %.2f (%.2fms)", fps, diff_ms);
  frames_without_time_update = 0;
}

cairo_surface_t*
create_cairo_surface (unsigned char *image)
{
  struct timespec now;
  if (clock_gettime (CLOCK_REALTIME, &now) != 0)
  {
    g_printerr ("Could not read system clock\n");
    return NULL;
  }

  double now_secs = openvr_time_to_double_secs (&now);

  cairo_surface_t *surface =
    cairo_image_surface_create_for_data (image,
                                         CAIRO_FORMAT_ARGB32,
                                         WIDTH, HEIGHT,
                                         STRIDE);

  cairo_t *cr = cairo_create (surface);

  cairo_set_source_rgba (cr, 0, 0, 0, 0);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);

  draw_rotated_quad (cr, WIDTH, now_secs);

  if (frames_without_time_update > 60)
    update_fps (&now);
  else
    frames_without_time_update++;

  last_time.tv_sec = now.tv_sec;
  last_time.tv_nsec = now.tv_nsec;

  draw_fps (cr, WIDTH, fps_str);

  cairo_destroy (cr);

  return surface;
}

gboolean
input_callback (gpointer data)
{
  OpenVROverlay *overlay = (OpenVROverlay*) data;
  openvr_overlay_poll_event (overlay);
  return TRUE;
}

struct RenderContext
{
  OpenVRVulkanUploader *uploader;
  OpenVROverlay *overlay;
};

gboolean
render_callback (gpointer data)
{
  struct RenderContext *context = (struct RenderContext*) data;

  /* render frame */
  unsigned char image[STRIDE*HEIGHT];
  cairo_surface_t* surface = create_cairo_surface (image);

  if (surface == NULL) {
    g_printerr ("Could not create cairo surface.\n");
    return FALSE;
  }

  OpenVRVulkanTexture *texture = openvr_vulkan_texture_new ();
  openvr_vulkan_client_load_cairo_surface (
    OPENVR_VULKAN_CLIENT (context->uploader),
    texture,
    surface);
  cairo_surface_destroy (surface);
  openvr_vulkan_uploader_submit_frame (context->uploader,
                                       context->overlay,
                                       texture);
  g_object_unref (texture);

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
_destroy_cb (OpenVROverlay *overlay,
             gpointer       data)
{
  (void) overlay;
  g_print ("destroy\n");
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}

bool
_init_openvr ()
{
  if (!openvr_context_is_installed ())
    {
      g_printerr ("VR Runtime not installed.\n");
      return false;
    }

  OpenVRContext *context = openvr_context_get_instance ();
  if (!openvr_context_init_overlay (context))
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  if (!openvr_context_is_valid (context))
    {
      g_printerr ("Could not load OpenVR function pointers.\n");
      return false;
    }

  return true;
}

int
test_overlay ()
{
  GMainLoop *loop;

  loop = g_main_loop_new (NULL, FALSE);

  struct RenderContext render_context;

  /* init openvr */
  if (!_init_openvr ())
    return -1;

  /* Upload vulkan texture */
  OpenVRVulkanUploader *uploader = openvr_vulkan_uploader_new ();
  if (!openvr_vulkan_uploader_init_vulkan (uploader, true))
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return -1;
  }

  /* create openvr overlay */
  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create (overlay, "examples.cairo", "Gradient",
                         ETrackingUniverseOrigin_TrackingUniverseStanding);

  if (!openvr_overlay_is_valid (overlay))
  {
    g_printerr ("Overlay unavailable.\n");
    return -1;
  }

  openvr_overlay_set_mouse_scale (overlay, (float) WIDTH, (float) HEIGHT);

  if (!openvr_overlay_show (overlay))
    return -1;

  g_signal_connect (overlay, "button-press-event", (GCallback) _press_cb, loop);
  g_signal_connect (overlay, "destroy", (GCallback) _destroy_cb, loop);

  render_context.overlay = overlay;
  render_context.uploader = uploader;

  g_timeout_add (20, input_callback, overlay);
  g_timeout_add (11, render_callback, &render_context); // sync to 90 hz

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_object_unref (overlay);
  g_object_unref (uploader);

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  return 0;
}

int main () {
  return test_overlay ();
}
