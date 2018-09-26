/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <stdint.h>
#include <gdk/gdkkeysyms.h>

GtkImage *image;
GdkPixbuf *pixbuf;

uint32_t width = 300;
uint32_t height = 300;

GtkApplication *app;

GdkPixbuf *
_create_empty_pixbuf (uint32_t width, uint32_t height)
{
  guchar * pixels = (guchar*) malloc (sizeof (guchar) * height * width * 4);
  memset (pixels, 10, height * width * 4 * sizeof (guchar));

  pixbuf = gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB,
                                     TRUE, 8, width, height,
                                     4 * width, NULL, NULL);
  return pixbuf;
}

gboolean
_key_cb (GtkWidget   *widget,
         GdkEventKey *event,
         gpointer     user_data)
{
  (void) widget;
  (void) user_data;

  if (event->keyval == GDK_KEY_Escape)
    g_application_quit (G_APPLICATION (app));

  return TRUE;
}

gboolean
_move_cb (GtkWidget      *widget,
          GdkEventMotion *event,
          gpointer        unused)
{
  (void) widget;
  (void) unused;

  /* check bounds */
  if (event->x < 0 || event->x > width || event->y < 0 || event->y > height)
    return TRUE;

  uint32_t x = (uint32_t) event->x;
  uint32_t y = (uint32_t) event->y;

  int n_channels = gdk_pixbuf_get_n_channels (pixbuf);
  int rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);
  guchar *p = pixels + y * rowstride + x * n_channels;

  p[0] = 245;
  p[1] = 245;
  p[2] = 245;
  p[3] = 255;

  gtk_image_set_from_pixbuf (image, pixbuf);

  return TRUE;
}

static void
activate (GtkApplication* app, gpointer user_data)
{
  (void) user_data;
  GtkWidget *window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Paint");

  GError *error = NULL;
  GdkPixbuf * pixbuf = _create_empty_pixbuf (300, 300);

  if (error != NULL) {
    g_printerr ("Unable to read file: %s\n", error->message);
    g_error_free (error);
    return;
  }

  image = (GtkImage*) gtk_image_new_from_pixbuf (pixbuf);

  g_signal_connect (window, "motion-notify-event", (GCallback) _move_cb, NULL);
  g_signal_connect (window, "key-press-event", (GCallback) _key_cb, NULL);

  gtk_container_add ((GtkContainer*) window, (GtkWidget*) image);

  gtk_widget_show_all (window);
}

int
main (int argc, char **argv)
{
  int status;

  app = gtk_application_new ("org.gtk.test", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
