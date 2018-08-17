/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <math.h>
#include <time.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include "cairo_content.h"
#include "openvr-time.h"

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

gboolean
draw (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  (void) data;

  struct timespec now;
  if (clock_gettime (CLOCK_REALTIME, &now) != 0)
  {
    g_printerr ("Could not read system clock\n");
    return TRUE;
  }

  double now_secs = openvr_time_to_double_secs (&now);

  guint width = gtk_widget_get_allocated_width (widget);

  draw_rotated_quad (cr, width, now_secs);

  if (frames_without_time_update > 60)
    update_fps (&now);
  else
    frames_without_time_update++;

  draw_fps (cr, width, fps_str);

  last_time.tv_sec = now.tv_sec;
  last_time.tv_nsec = now.tv_nsec;

  gtk_widget_queue_draw (widget);

  return FALSE;
}

static void
activate (GtkApplication* app, gpointer user_data)
{
  (void) user_data;
  GtkWidget *window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Cairo");

  GtkWidget *drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawing_area, 1000, 1000);
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
