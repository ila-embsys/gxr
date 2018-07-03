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

struct timespec last_time;
unsigned frames_without_time_update = 0;
gchar fps_str [50];

#define SEC_IN_MSEC_D 1000.0
#define SEC_IN_NSEC_L 1000000000L
#define SEC_IN_NSEC_D 1000000000.0

/* assuming a > b */
void
_substract_timespecs (struct timespec* a,
                      struct timespec* b,
                      struct timespec* out)
{
  out->tv_sec = a->tv_sec - b->tv_sec;

  if (a->tv_nsec < b->tv_nsec)
  {
    out->tv_nsec = a->tv_nsec + SEC_IN_NSEC_L - b->tv_nsec;
    out->tv_sec--;
  }
  else
  {
    out->tv_nsec = a->tv_nsec - b->tv_nsec;
  }
}

double
_timespec_to_double_s (struct timespec* time)
{
  return ((double) time->tv_sec + (time->tv_nsec / SEC_IN_NSEC_D));
}

gboolean
draw (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  struct timespec now;
  if (clock_gettime (CLOCK_REALTIME, &now) != 0)
  {
    fprintf (stderr, "Could not read system clock\n");
    return TRUE;
  }

  struct timespec diff;
  _substract_timespecs (&now, &last_time, &diff);

  double now_secs = _timespec_to_double_s (&now);

  double diff_s = _timespec_to_double_s (&diff);
  double diff_ms = diff_s * SEC_IN_MSEC_D;
  double fps = SEC_IN_MSEC_D / diff_ms;

  guint width = gtk_widget_get_allocated_width (widget);

  draw_rotated_quad (cr, width, now_secs);

  if (frames_without_time_update > 60)
    {
      frames_without_time_update = 0;
      g_sprintf (fps_str, "FPS %.2f (%.2fms)", fps, diff_ms);
    }
  else
    {
      frames_without_time_update += 1;
    }

  draw_fps (cr, width, fps_str);

  gtk_widget_queue_draw (widget);

  last_time.tv_sec = now.tv_sec;
  last_time.tv_nsec = now.tv_nsec;

  return FALSE;
}

static void
activate (GtkApplication* app, gpointer user_data)
{
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
