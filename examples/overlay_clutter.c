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

#include "openvr-glib.h"

#include <cairo.h>
#include <clutter/clutter.h>
#include "clutter_content.h"

GulkanTexture *texture = NULL;
OpenVROverlayUploader *uploader;

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

#define ALIGN(_v, _d) (((_v) + ((_d) - 1)) & ~((_d) - 1))

static gboolean
repaint_cb (gpointer user_data)
{
  Data *data = (Data*) user_data;

  /* skip rendering if the overlay isn't available or visible */
  gboolean is_invisible = !openvr_overlay_is_visible (data->overlay) &&
                          !openvr_overlay_thumbnail_is_visible (data->overlay);

  if (!openvr_overlay_is_valid (data->overlay) || is_invisible)
    return FALSE;

  uint32_t width = 500;
  uint32_t height = 500;
  int stride = ALIGN ((int) width, 32) * 4;
  uint64_t size = (uint64_t) stride * height;

  guchar* pixels = clutter_stage_read_pixels
    (CLUTTER_STAGE(data->stage), 0, 0, width, height);

  GulkanClient *client = GULKAN_CLIENT (uploader);
  GulkanDevice *device = gulkan_client_get_device (client);

  if (texture == NULL)
    texture = gulkan_texture_new (device,
                                  width, height, size,
                                  VK_FORMAT_R8G8B8A8_UNORM);

  gulkan_client_upload_pixels (client, texture, pixels,size,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  openvr_overlay_uploader_submit_frame (uploader, data->overlay, texture);

  return TRUE;
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
test_cat_overlay (int argc, char *argv[])
{
  GMainLoop *loop;

  loop = g_main_loop_new (NULL, FALSE);

  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    {
      fprintf (stderr, "Failed to initialize clutter.\n");
      return EXIT_FAILURE;
    }

  /* init openvr */
  if (!_init_openvr ())
    return -1;

  uploader = openvr_overlay_uploader_new ();
  if (!openvr_overlay_uploader_init_vulkan (uploader, true))
    {
      g_printerr ("Unable to initialize Vulkan!\n");
      return false;
    }

  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create_width (overlay, "example.clutter", "Clutter", 2.0);

  if (!openvr_overlay_is_valid (overlay))
    {
      fprintf (stderr, "Overlay unavailable.\n");
      return -1;
    }

  graphene_point3d_t pos = { .x = 0, .y = 1, .z = -2 };
  openvr_overlay_set_translation (overlay, &pos);
  openvr_overlay_show (overlay);

  openvr_overlay_set_mouse_scale (overlay, 500.0f, 500.0f);

  g_signal_connect (overlay, "button-press-event", (GCallback) _press_cb, loop);
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

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  return 0;
}

int
main (int argc, char *argv[])
{
  return test_cat_overlay (argc, argv);
}
