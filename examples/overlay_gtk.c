/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <time.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <openvr-glib.h>

#include "openvr-system.h"
#include "openvr-overlay.h"

#include <GL/glut.h>

static gboolean
_damage_cb (GtkWidget *widget, GdkEventExpose *event, OpenVROverlay *overlay)
{
  GdkPixbuf * offscreen_pixbuf =
    gtk_offscreen_window_get_pixbuf ((GtkOffscreenWindow *)widget);

  if (offscreen_pixbuf != NULL)
  {
    /* skip rendering if the overlay isn't available or visible */
    gboolean is_unavailable = !openvr_overlay_is_valid (overlay) ||
                              !openvr_system_is_overlay_available ();
    gboolean is_invisible = !openvr_overlay_is_visible (overlay) &&
                            !openvr_overlay_thumbnail_is_visible (overlay);

    if (is_unavailable || is_invisible)
    {
      g_object_unref (offscreen_pixbuf);
      return TRUE;
    }

    GdkPixbuf *pixbuf_flipped = gdk_pixbuf_flip (offscreen_pixbuf, FALSE);
    g_object_unref (offscreen_pixbuf);

    openvr_overlay_upload_gdk_pixbuf (overlay, pixbuf_flipped);
    g_object_unref (pixbuf_flipped);
  } else {
    fprintf (stderr, "Could not acquire pixbuf.\n");
  }

  return TRUE;
}

static gboolean
_button_press_cb (GtkWidget *button, GdkEventButton *event, gpointer data)
{
  g_print ("button pressed.\n");
  return TRUE;
}

struct Labels
{
  GtkWidget *time_label;
  GtkWidget *fps_label;
  struct timespec last_time;
};

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

static gboolean
_draw_cb (GtkWidget *widget, cairo_t *cr, struct Labels* labels)
{
  struct timespec now;
  if (clock_gettime (CLOCK_REALTIME, &now) != 0)
  {
    fprintf (stderr, "Could not read system clock\n");
    return 0;
  }

  struct timespec diff;
  _substract_timespecs (&now, &labels->last_time, &diff);

  double diff_s = _timespec_to_double_s (&diff);
  double diff_ms = diff_s * SEC_IN_MSEC_D;
  double fps = SEC_IN_MSEC_D / diff_ms;

  gchar time_str [50];
  gchar fps_str [50];

  g_sprintf (time_str, "<span font=\"24\">%.2ld:%.9ld</span>",
             now.tv_sec, now.tv_nsec);

  g_sprintf (fps_str, "FPS %.2f (%.2fms)", fps, diff_ms);

  gtk_label_set_markup (GTK_LABEL (labels->time_label), time_str);
  gtk_label_set_text (GTK_LABEL (labels->fps_label), fps_str);

  labels->last_time.tv_sec = now.tv_sec;
  labels->last_time.tv_nsec = now.tv_nsec;

  return FALSE;
}

gboolean
timeout_callback (gpointer data)
{
  OpenVROverlay *overlay = (OpenVROverlay*) data;
  openvr_overlay_poll_event (overlay);
  return TRUE;
}

void
print_gl_context_info ()
{
  const GLubyte *version = glGetString (GL_VERSION);
  const GLubyte *vendor = glGetString (GL_VENDOR);
  const GLubyte *renderer = glGetString (GL_RENDERER);

  GLint major;
  GLint minor;
  glGetIntegerv (GL_MAJOR_VERSION, &major);
  glGetIntegerv (GL_MINOR_VERSION, &minor);

  g_print ("%s %s %s (%d.%d)\n", vendor, renderer, version, major, minor);
}


gboolean
init_offscreen_gl (int argc, char *argv[])
{
  /* TODO: GLFW conflicts here with Gtk, glut works */
  glutInit (&argc, argv);
  glutInitDisplayMode (GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
  glutInitWindowSize (100,  100);
  glutCreateWindow ("glut");

  return TRUE;
}

static void
_move_cb (OpenVROverlay  *overlay,
          GdkEventMotion *event,
          gpointer        data)
{
  g_print ("move: %f %f (%d)\n", event->x, event->y, event->time);
}

static void
_press_cb (OpenVROverlay  *overlay,
           GdkEventButton *event,
           gpointer        data)
{
  g_print ("press: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}

static void
_release_cb (OpenVROverlay  *overlay,
             GdkEventButton *event,
             gpointer        data)
{
  g_print ("release: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
}

static void
_show_cb (OpenVROverlay *overlay,
          gpointer       data)
{
  g_print ("show\n");
}

static void
_destroy_cb (OpenVROverlay *overlay,
             gpointer       data)
{
  g_print ("destroy\n");
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}

int main (int argc, char *argv[]) {
  GMainLoop *loop;

  gtk_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);

  GtkWidget *window = gtk_offscreen_window_new ();

  struct Labels labels;

  if (clock_gettime (CLOCK_REALTIME, &labels.last_time) != 0)
  {
    fprintf (stderr, "Could not read system clock\n");
    return 0;
  }

  labels.time_label = gtk_label_new ("");
  labels.fps_label = gtk_label_new ("");

  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  GtkWidget *button = gtk_button_new_with_label ("Button");

  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), labels.time_label, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), labels.fps_label, FALSE, FALSE, 0);

  gtk_widget_set_size_request (window , 300, 200);
  gtk_container_add (GTK_CONTAINER (window), box);

  gtk_widget_show_all (window);

  init_offscreen_gl (argc, argv);

  print_gl_context_info ();

  OpenVRSystem * system = openvr_system_new ();
  gboolean ret = openvr_system_init_overlay (system);

  g_assert (ret);
  g_assert (openvr_system_is_available ());
  g_assert (openvr_system_is_compositor_available ());

  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create (overlay, "openvr.example.gtk", "GTK+");

  if (!openvr_overlay_is_valid (overlay) ||
      !openvr_system_is_overlay_available ())
  {
    fprintf (stderr, "Overlay unavailable.\n");
    return -1;
  }

  openvr_overlay_set_mouse_scale (overlay, 300.0f, 200.0f);

  g_signal_connect (overlay, "motion-notify-event", (GCallback) _move_cb, NULL);
  g_signal_connect (overlay, "button-press-event", (GCallback) _press_cb, loop);
  g_signal_connect (overlay, "button-release-event",
                    (GCallback) _release_cb, NULL);
  g_signal_connect (overlay, "show", (GCallback) _show_cb, NULL);
  g_signal_connect (overlay, "destroy", (GCallback) _destroy_cb, loop);

  g_signal_connect (window, "damage-event", G_CALLBACK (_damage_cb), overlay);
  g_signal_connect (button, "button_press_event",
                    G_CALLBACK (_button_press_cb), NULL);
  g_signal_connect (window, "draw", G_CALLBACK (_draw_cb), &labels);

  g_timeout_add (20, timeout_callback, overlay);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_object_unref (overlay);

  return 0;
}
