/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <math.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

gboolean
draw (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  guint width = gtk_widget_get_allocated_width (widget);
  guint height = gtk_widget_get_allocated_height (widget);

  cairo_pattern_t *pat;
  pat = cairo_pattern_create_linear (0.0, 0.0,  0.0, height);
  cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0, 0, 1);
  cairo_pattern_add_color_stop_rgba (pat, 0, 1, 1, 1, 1);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  double center_x = (double) width / 2.0;
  double center_y = (double) height / 2.0;

  double r0;
  if (width < height)
    r0 = (double) width / 10.0;
  else
    r0 = (double) height / 10.0;

  double radius = r0 * 3.0;
  double r1 = r0 * 5.0;

  double cx0 = center_x - r0 / 2.0;
  double cy0 = center_y - r0;
  double cx1 = center_x - r0;
  double cy1 = center_y - r0;

  pat = cairo_pattern_create_radial (cx0, cy0, r0,
                                     cx1, cy1, r1);
  cairo_pattern_add_color_stop_rgba (pat, 0, 1, 1, 1, 1);
  cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0, 0, 1);
  cairo_set_source (cr, pat);
  cairo_arc (cr, center_x, center_y, radius, 0, 2 * M_PI);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

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
