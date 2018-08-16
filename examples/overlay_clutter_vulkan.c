/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <GL/gl.h>

#include <math.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

#include <openvr-glib.h>

#include "openvr-system.h"
#include "openvr-overlay.h"

#include <cairo.h>
#include <clutter/clutter.h>
#include "clutter_content.h"

#include "openvr-vulkan-uploader.h"

OpenVRVulkanTexture *texture;
OpenVRVulkanUploader *uploader;

gboolean
timeout_callback (gpointer data)
{
  OpenVROverlay *overlay = (OpenVROverlay*) data;
  openvr_overlay_poll_event (overlay);
  return TRUE;
}

typedef struct
{
  ClutterActor *stage;
  OpenVROverlay *overlay;
} Data;

void
print_pixbuf_info (GdkPixbuf * pixbuf)
{
  gint n_channels = gdk_pixbuf_get_n_channels (pixbuf);

  GdkColorspace colorspace = gdk_pixbuf_get_colorspace (pixbuf);

  gint bits_per_sample = gdk_pixbuf_get_bits_per_sample (pixbuf);
  gboolean has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);

  gint width = gdk_pixbuf_get_width (pixbuf);
  gint height = gdk_pixbuf_get_height (pixbuf);

  gint rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  g_print ("We loaded a pixbuf.\n");
  g_print ("channels %d\n", n_channels);
  g_print ("colorspace %d\n", colorspace);
  g_print ("bits_per_sample %d\n", bits_per_sample);
  g_print ("has_alpha %d\n", has_alpha);
  g_print ("pixel dimensions %dx%d\n", width, height);
  g_print ("rowstride %d\n", rowstride);
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
}

static void
_destroy_cb (OpenVROverlay *overlay,
             gpointer       data)
{
  g_print ("destroy\n");
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}

#define ALIGN(_v, _d) (((_v) + ((_d) - 1)) & ~((_d) - 1))

static gboolean
repaint_cb (gpointer user_data)
{
  Data *data = (Data*) user_data;

  /* skip rendering if the overlay isn't available or visible */
  gboolean is_unavailable = !openvr_overlay_is_valid (data->overlay) ||
                            !openvr_overlay_is_available (data->overlay);
  gboolean is_invisible = !openvr_overlay_is_visible (data->overlay) &&
                          !openvr_overlay_thumbnail_is_visible (data->overlay);

  if (is_unavailable || is_invisible)
    return TRUE;

  uint32_t width = 500;
  uint32_t height = 500;
  int stride = ALIGN ((int) width, 32) * 4;
  uint64_t size = (uint64_t) stride * height;

  guchar* pixels = clutter_stage_read_pixels
    (CLUTTER_STAGE(data->stage), 0, 0, width, height);

  openvr_vulkan_client_load_raw (OPENVR_VULKAN_CLIENT (uploader),
                                 texture, pixels,
                                 width, height, size,
                                 VK_FORMAT_R8G8B8A8_UNORM);

  openvr_vulkan_uploader_submit_frame (uploader, data->overlay, texture);

  return TRUE;
}

int
test_cat_overlay (int argc, char *argv[])
{
  GMainLoop *loop;

  loop = g_main_loop_new (NULL, FALSE);

  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    {
      fprintf (stderr, "Failed to initialize clutter.\n");
      return EXIT_FAILURE;
    }

  OpenVRSystem * system = openvr_system_new ();
  gboolean ret = openvr_system_init_overlay (system);
  OpenVRCompositor *compositor = openvr_compositor_new ();

  g_assert (ret);
  g_assert (openvr_system_is_available (system));
  g_assert (openvr_system_is_installed ());

  uploader = openvr_vulkan_uploader_new ();
  if (!openvr_vulkan_uploader_init_vulkan (uploader, true, system, compositor))
    {
      g_printerr ("Unable to initialize Vulkan!\n");
      return false;
    }

  texture = openvr_vulkan_texture_new ();

  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create_for_dashboard (overlay, "example.clutter", "Clutter");

  if (!openvr_overlay_is_valid (overlay) ||
      !openvr_overlay_is_available (overlay))
    {
      fprintf (stderr, "Overlay unavailable.\n");
      return -1;
    }

  openvr_overlay_set_mouse_scale (overlay, 500.0f, 500.0f);

  g_signal_connect (overlay, "motion-notify-event", (GCallback) _move_cb, NULL);
  g_signal_connect (overlay, "button-press-event", (GCallback) _press_cb, loop);
  g_signal_connect (
    overlay, "button-release-event", (GCallback) _release_cb, NULL);
  g_signal_connect (overlay, "show", (GCallback) _show_cb, NULL);
  g_signal_connect (overlay, "destroy", (GCallback) _destroy_cb, loop);

  g_timeout_add (20, timeout_callback, overlay);

  ClutterActor * stage = clutter_stage_new ();
  create_rotated_quad_stage (stage);

  //clutter_actor_set_offscreen_redirect (stage,
  //                                      CLUTTER_OFFSCREEN_REDIRECT_ALWAYS);

  clutter_actor_show (stage);

  Data data;
  data.stage = stage;
  data.overlay = overlay;

  clutter_threads_add_repaint_func_full (CLUTTER_REPAINT_FLAGS_POST_PAINT,
                                         repaint_cb,
                                         &data,
                                         NULL);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_object_unref (overlay);
  g_object_unref (texture);
  g_object_unref (uploader);

  return 0;
}

int
main (int argc, char *argv[])
{
  return test_cat_overlay (argc, argv);
}
