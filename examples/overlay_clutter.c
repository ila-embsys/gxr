/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <GL/gl.h>

#include <math.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

#include "gxr.h"

#include <cairo.h>

#define COGL_ENABLE_EXPERIMENTAL_2_0_API 1
#include <clutter/clutter.h>
#include "clutter_content.h"

static GulkanTexture *texture = NULL;
static GulkanClient *uploader;

static gboolean
timeout_callback (gpointer data)
{
  GxrOverlay *overlay = (GxrOverlay*) data;
  gxr_overlay_poll_event (overlay);
  return TRUE;
}

typedef struct
{
  ClutterActor *stage;
  GxrOverlay *overlay;
} Data;

#if 0
static void
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
#endif

static void
_press_cb (GxrOverlay  *overlay,
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
_destroy_cb (GxrOverlay *overlay,
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
  gboolean is_invisible = !gxr_overlay_is_visible (data->overlay) &&
                          !gxr_overlay_thumbnail_is_visible (data->overlay);

  if (!gxr_overlay_is_valid (data->overlay) || is_invisible)
    return FALSE;

  uint32_t width = 500;
  uint32_t height = 500;
  int stride = ALIGN ((int) width, 32) * 4;
  uint64_t size = (uint64_t) stride * height;

  guchar* pixels = clutter_stage_read_pixels
    (CLUTTER_STAGE(data->stage), 0, 0, (gint)width, (gint)height);

  GulkanClient *client = GULKAN_CLIENT (uploader);
  GulkanDevice *device = gulkan_client_get_device (client);

  if (texture == NULL)
    texture = gulkan_texture_new (device,
                                  width, height, size,
                                  VK_FORMAT_R8G8B8A8_UNORM);

  gulkan_client_upload_pixels (client, texture, pixels,size,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  gxr_overlay_submit_texture (data->overlay, uploader, texture);

  return TRUE;
}

static int
test_cat_overlay (int argc, char *argv[])
{
  GMainLoop *loop;

  loop = g_main_loop_new (NULL, FALSE);

  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    {
      fprintf (stderr, "Failed to initialize clutter.\n");
      return EXIT_FAILURE;
    }

  GxrContext *context = gxr_context_new ();
  uploader = gulkan_client_new ();

  if (!gxr_context_inititalize (context, uploader, GXR_APP_OVERLAY))
    return -1;

  GxrOverlay *overlay = gxr_overlay_new_width (context, "example.clutter", 2.0);
  if (!gxr_overlay_is_valid (overlay))
    {
      fprintf (stderr, "Overlay unavailable.\n");
      return -1;
    }

  graphene_point3d_t pos = { .x = 0, .y = 1, .z = -2 };
  gxr_overlay_set_translation (overlay, &pos);
  gxr_overlay_show (overlay);

  gxr_overlay_set_mouse_scale (overlay, 500.0f, 500.0f);

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

  g_object_unref (context);

  return 0;
}

int
main (int argc, char *argv[])
{
  return test_cat_overlay (argc, argv);
}
