/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <math.h>
#include <gtk/gtk.h>

#include "cairo_content.h"

gboolean
draw (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  guint width = gtk_widget_get_allocated_width (widget);
  guint height = gtk_widget_get_allocated_height (widget);

  draw_gradient_quad (cr, width, height);
  draw_gradient_circle (cr, width, height);

  return FALSE;
}

static void
activate (GtkApplication* app, gpointer user_data)
{
  GtkWidget *window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Cairo");

  GtkWidget *drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawing_area, 1280, 720);
  g_signal_connect (G_OBJECT (drawing_area), "draw",
                    G_CALLBACK (draw), NULL);

  gtk_container_add ((GtkContainer*) window, drawing_area);

  gtk_widget_show_all (window);
}

int
main (int argc, char **argv)
{
  GtkApplication *app = gtk_application_new ("org.openvr-glib.examples.cairo",
                                             G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  int status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
