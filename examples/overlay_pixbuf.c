/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <glib-unix.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

#include "gxr.h"

static GulkanTexture *texture;

static gboolean
_poll_cb (gpointer data)
{
  GxrOverlay *overlay = (GxrOverlay*) data;
  gxr_overlay_poll_event (overlay);
  return TRUE;
}

static gboolean
_sigint_cb (gpointer _loop)
{
  GMainLoop *loop = (GMainLoop*) _loop;
  g_main_loop_quit (loop);
  return TRUE;
}

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

static GdkPixbuf *
load_gdk_pixbuf ()
{
  GError *error = NULL;
  GdkPixbuf * pixbuf_unflipped =
    gdk_pixbuf_new_from_resource ("/res/cat.jpg", &error);

  if (error != NULL) {
    g_printerr ("Unable to read file: %s\n", error->message);
    g_error_free (error);
    return NULL;
  } else {
    //GdkPixbuf * pixbuf = gdk_pixbuf_flip (pixbuf_unflipped, FALSE);
    GdkPixbuf *pixbuf = gdk_pixbuf_add_alpha (pixbuf_unflipped, false, 0, 0, 0);
    g_object_unref (pixbuf_unflipped);
    //g_object_unref (pixbuf);
    return pixbuf;
  }
}

static void
_move_cb (GxrOverlay  *overlay,
          GdkEventMotion *event,
          gpointer        data)
{
  (void) overlay;
  (void) data;
  g_print ("move: %f %f (%d)\n", event->x, event->y, event->time);
}

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
_release_cb (GxrOverlay  *overlay,
             GdkEventButton *event,
             gpointer        data)
{
  (void) overlay;
  (void) data;
  g_print ("release: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
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

static int
test_cat_overlay ()
{
  GMainLoop *loop;

  GdkPixbuf * pixbuf = load_gdk_pixbuf ();
  print_pixbuf_info (pixbuf);

  if (pixbuf == NULL)
    return -1;

  loop = g_main_loop_new (NULL, FALSE);

  GxrContext *context = gxr_context_new ();
  GulkanClient *client = gulkan_client_new ();

  if (!gxr_context_inititalize (context, client, GXR_APP_OVERLAY))
    return -1;

  texture = gulkan_client_texture_new_from_pixbuf (client, pixbuf,
                                                   VK_FORMAT_R8G8B8A8_UNORM,
                                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                   true);

  GxrOverlay *overlay = gxr_overlay_new (context);
  gxr_overlay_create_width (overlay, "vulkan.cat", "Vulkan Cat", 2.0f);

  if (!gxr_overlay_is_valid (overlay))
  {
    fprintf (stderr, "Overlay unavailable.\n");
    return -1;
  }

  gxr_overlay_set_mouse_scale (overlay,
                                  gdk_pixbuf_get_width (pixbuf),
                                  gdk_pixbuf_get_height (pixbuf));

  gxr_overlay_submit_texture (overlay, client, texture);

  graphene_matrix_t transform;
  graphene_point3d_t pos =
  {
    .x = 0,
    .y = 1,
    .z = -2
  };
  graphene_matrix_init_translate (&transform, &pos);
  gxr_overlay_set_transform_absolute (overlay, &transform);

  gxr_overlay_show (overlay);

  g_signal_connect (overlay, "motion-notify-event", (GCallback) _move_cb, NULL);
  g_signal_connect (overlay, "button-press-event", (GCallback) _press_cb, loop);
  g_signal_connect (
    overlay, "button-release-event", (GCallback) _release_cb, NULL);
  g_signal_connect (overlay, "destroy", (GCallback) _destroy_cb, loop);

  g_timeout_add (20, _poll_cb, overlay);

  g_unix_signal_add (SIGINT, _sigint_cb, loop);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_print ("bye\n");

  g_object_unref (overlay);
  g_object_unref (pixbuf);
  g_object_unref (texture);

  g_object_unref (context);

  g_object_unref (client);

  return 0;
}

int
main ()
{
  return test_cat_overlay ();
}
