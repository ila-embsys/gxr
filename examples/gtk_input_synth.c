/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <time.h>
#include <math.h>

#include <X11/Xlib.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

static gboolean
_quit_cb (GtkWidget *button, GdkEvent *event, GMainLoop *loop)
{
  g_main_loop_quit (loop);
  return TRUE;
}

static gboolean
_quit2_cb (GtkWidget *button, GMainLoop *loop)
{
  g_main_loop_quit (loop);
  return TRUE;
}

static gdouble last_mouse_x;
static gdouble last_mouse_y;

gboolean
_move_cb (GtkWidget      *widget,
          GdkEventMotion *event,
          gpointer        user_data)
{
  g_print ("%s move: %f %f (%d) (%.2fx%.2f)\n",
           gtk_widget_get_name (widget),
           event->x, event->y,
           event->time,
           event->x_root, event->y_root);

  /* TODO: lock this */
  last_mouse_x = event->x;
  last_mouse_y = event->y;

  return TRUE;
}

#define SEC_IN_NSEC_D 1000000000.0
#define SEC_IN_MSEC_D 1000.0

guint32
monotonic_time_ms ()
{
  struct timespec now;
  if (clock_gettime (CLOCK_MONOTONIC, &now) != 0)
  {
    fprintf (stderr, "Could not read system clock\n");
    return 0;
  }

  gdouble now_s = now.tv_sec + (now.tv_nsec / SEC_IN_NSEC_D);

  return (guint32) (now_s * SEC_IN_MSEC_D);
}

void
synthesize_gtk_motion_event (GdkWindow *window, gdouble x, gdouble y)
{
  GdkEvent *event = gdk_event_new (GDK_MOTION_NOTIFY);
  event->motion.x = x;
  event->motion.y = y;

  event->motion.time = monotonic_time_ms ();

  event->any.send_event = TRUE;
  event->any.window = g_object_ref(window);
  event->motion.window = g_object_ref(window);
  GdkDisplay *display = gdk_window_get_display (window);
  GdkSeat *seat = gdk_display_get_default_seat (display);

  event->motion.device = gdk_seat_get_pointer (seat);

  gint wx, wy;
  gdk_window_get_origin (window, &wx, &wy);

  event->motion.x_root = event->motion.x + wx;
  event->motion.y_root = event->motion.y + wy;

  gtk_main_do_event (event);
}

struct GdkSimulation
{
  guint runs;
  guint32 start_time_ms;
  gdouble start_x;
  gdouble start_y;
};

static struct GdkSimulation gdk_simulation;

gboolean
_gtk_synth_iteration_cb (gpointer data)
{
  GdkWindow *window = GDK_WINDOW (data);

  guint32 now = monotonic_time_ms ();
  guint32 passed = now - gdk_simulation.start_time_ms;

  gdouble passed_sin = sin (((gdouble) passed) / 500.0);

  gdk_simulation.runs += 1;

  gdouble current_y = gdk_simulation.start_y + passed_sin * 100.0;

  synthesize_gtk_motion_event (window, gdk_simulation.start_x, current_y);

  if (gdk_simulation.runs > 50)
    return FALSE;
  else
    return TRUE;
}

static gboolean
_run_gtk_synth_cb (GtkWidget *button, GdkEventButton *event, GdkWindow *window)
{
  gdk_simulation.runs = 0;
  gdk_simulation.start_time_ms = monotonic_time_ms ();

  /* TODO: lock this */
  gdk_simulation.start_x = last_mouse_x;
  gdk_simulation.start_y = last_mouse_y;

  g_timeout_add (100, _gtk_synth_iteration_cb, window);
  return TRUE;
}

void
synthesize_xlib_motion_event (GdkWindow *window, gdouble x, gdouble y)
{
  GdkDisplay *display = gdk_window_get_display (window);
  Display * x_display = gdk_x11_display_get_xdisplay (display);
  Window x_window = gdk_x11_window_get_xid (window);

  XSelectInput(x_display, x_window, KeyReleaseMask);
  XWarpPointer(x_display, None, x_window, 0, 0, 0, 0, (int) x, (int) y);
  XSync(x_display, FALSE);
}

gboolean
_xlib_synth_iteration_cb (gpointer data)
{
  GdkWindow *window = GDK_WINDOW (data);

  guint32 now = monotonic_time_ms ();
  guint32 passed = now - gdk_simulation.start_time_ms;

  gdouble passed_sin = sin (((gdouble) passed) / 500.0);

  gdk_simulation.runs += 1;

  gdouble current_y = gdk_simulation.start_y + passed_sin * 100.0;

  synthesize_xlib_motion_event (window, gdk_simulation.start_x, current_y);

  if (gdk_simulation.runs > 50)
    return FALSE;
  else
    return TRUE;
}

static gboolean
_run_xlib_synth_cb (GtkWidget *button, GdkEventButton *event, GdkWindow *window)
{
  gdk_simulation.runs = 0;
  gdk_simulation.start_time_ms = monotonic_time_ms ();

  /* TODO: lock this */
  gdk_simulation.start_x = last_mouse_x;
  gdk_simulation.start_y = last_mouse_y;

  g_timeout_add (100, _xlib_synth_iteration_cb, window);
  return TRUE;
}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  GMainLoop *loop = g_main_loop_new (NULL, FALSE);

  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  GtkWidget *button1 = gtk_button_new_with_label ("Synth Gtk");
  GtkWidget *button2 = gtk_button_new_with_label ("Synth Xlib");
  GtkWidget *button3 = gtk_button_new_with_label ("Quit");

  gtk_box_pack_start (GTK_BOX (box), button1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), button2, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), button3, FALSE, FALSE, 0);

  g_object_set (G_OBJECT (box), "margin", 20, NULL);

  gtk_widget_set_size_request (window , 300, 200);
  gtk_container_add (GTK_CONTAINER (window), box);

  gtk_widget_show_all (window);

  GdkWindow *gdk_window = gtk_widget_get_window (window);

  g_signal_connect (button1, "button-press-event",
                    (GCallback) _run_gtk_synth_cb, gdk_window);
  g_signal_connect (button2, "button-press-event",
                    (GCallback) _run_xlib_synth_cb, gdk_window);
  g_signal_connect (button3, "button-press-event", (GCallback) _quit_cb, loop);
  g_signal_connect (window, "destroy", (GCallback) _quit2_cb, loop);

  g_signal_connect (window, "motion-notify-event", (GCallback) _move_cb, NULL);
  g_signal_connect (button1, "motion-notify-event", (GCallback) _move_cb, NULL);
  g_signal_connect (button2, "motion-notify-event", (GCallback) _move_cb, NULL);
  g_signal_connect (button3, "motion-notify-event", (GCallback) _move_cb, NULL);

  gtk_widget_add_events (window, GDK_POINTER_MOTION_MASK);
  gtk_widget_add_events (button1, GDK_POINTER_MOTION_MASK);
  gtk_widget_add_events (button2, GDK_POINTER_MOTION_MASK);
  gtk_widget_add_events (button3, GDK_POINTER_MOTION_MASK);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  return 0;
}
