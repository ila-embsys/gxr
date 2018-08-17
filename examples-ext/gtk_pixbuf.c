/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

static void
activate (GtkApplication* app, gpointer user_data)
{
  (void) user_data;
  GtkWidget *window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Cat");

  GError *error = NULL;
  GdkPixbuf * pixbuf = gdk_pixbuf_new_from_resource ("/res/cat.jpg", &error);

  if (error != NULL) {
    fprintf (stderr, "Unable to read file: %s\n", error->message);
    g_error_free (error);
  } else {

    int width = gdk_pixbuf_get_width (pixbuf);
    int height = gdk_pixbuf_get_height (pixbuf);

    GdkPixbuf * scaled_pixbuf =
      gdk_pixbuf_scale_simple (pixbuf,
                               width / 2,
                               height / 2,
                               GDK_INTERP_BILINEAR);

    GtkWidget * image = gtk_image_new_from_pixbuf (scaled_pixbuf);
    gtk_container_add ((GtkContainer*) window, image);
    g_object_unref (pixbuf);
    g_object_unref (scaled_pixbuf);
  }

  gtk_widget_show_all (window);
}

int
main (int argc, char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.test", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
