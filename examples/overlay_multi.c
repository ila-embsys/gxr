/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <graphene.h>

#include "gxr.h"

static GulkanTexture *texture;

static gboolean
timeout_callback (gpointer data)
{
  GxrOverlay *overlay = (GxrOverlay*) data;
  gxr_overlay_poll_event (overlay);
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
    fprintf (stderr, "Unable to read file: %s\n", error->message);
    g_error_free (error);
    return NULL;
  } else {
    //GdkPixbuf * pixbuf = gdk_pixbuf_flip (pixbuf_unflipped, FALSE);
    GdkPixbuf *pixbuf = gdk_pixbuf_add_alpha (pixbuf_unflipped, FALSE, 0, 0, 0);
    g_object_unref (pixbuf_unflipped);
    //g_object_unref (pixbuf);
    return pixbuf;
  }
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
_show_cb (GxrOverlay *overlay,
          gpointer    unused)
{
  (void) unused;
  g_print ("show\n");

  /* skip rendering if the overlay isn't available or visible */
  gboolean is_invisible = !gxr_overlay_is_visible (overlay) &&
                          !gxr_overlay_thumbnail_is_visible (overlay);

  if (is_invisible)
    return;

  gxr_overlay_submit_texture (overlay, texture);
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

  GxrContext *context = gxr_context_new (GXR_APP_OVERLAY);
  GulkanClient *client = gxr_context_get_gulkan (context);

  texture = gulkan_texture_new_from_pixbuf (client, pixbuf,
                                            VK_FORMAT_R8G8B8A8_UNORM,
                                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                            TRUE);

  GxrOverlay *overlay = gxr_overlay_new (context, "vulkan.cat");
  GxrOverlay *overlay2 = gxr_overlay_new (context, "vulkan.cat2");
  if (overlay == NULL || overlay2 == NULL)
  {
    g_error ("Could not create overlays.");
    return -1;
  }

  gxr_overlay_set_mouse_scale (overlay,
                               (float) (gdk_pixbuf_get_width (pixbuf)),
                               (float) (gdk_pixbuf_get_height (pixbuf)));

  if (!gxr_overlay_show (overlay))
    return -1;

  if (!gxr_overlay_show (overlay2))
    return -1;

  gxr_overlay_print_info (overlay);

  graphene_point3d_t translation_vec;
  graphene_point3d_init (&translation_vec, 1.1f, 0.5f, 0.1f);

  graphene_matrix_t translation;
  graphene_matrix_init_translate (&translation, &translation_vec);
  graphene_matrix_rotate_y (&translation, -30.4f);

  graphene_matrix_print (&translation);

  gxr_overlay_get_transform_absolute (overlay, &translation);

  gxr_overlay_submit_texture (overlay, texture);
  gxr_overlay_submit_texture (overlay2, texture);

  /* connect glib callbacks */
  g_signal_connect (overlay, "button-press-event", (GCallback) _press_cb, loop);
  g_signal_connect (overlay, "show", (GCallback) _show_cb, NULL);
  g_signal_connect (overlay, "destroy", (GCallback) _destroy_cb, loop);

  g_timeout_add (20, timeout_callback, overlay);

  /* start glib main loop */
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_print ("bye\n");

  g_object_unref (overlay);
  g_object_unref (pixbuf);
  g_object_unref (texture);
  g_object_unref (context);

  return 0;
}

int main () {
  return test_cat_overlay ();
}
