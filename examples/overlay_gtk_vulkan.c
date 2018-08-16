/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <time.h>
#include <GL/gl.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <openvr-glib.h>

#include "openvr-context.h"
#include "openvr-overlay.h"
#include "openvr-time.h"
#include "openvr-vulkan-uploader.h"

OpenVRVulkanTexture *texture;
OpenVRVulkanUploader *uploader;

static gboolean
_damage_cb (GtkWidget *widget, GdkEventExpose *event, OpenVROverlay *overlay)
{
  GdkPixbuf * offscreen_pixbuf =
    gtk_offscreen_window_get_pixbuf ((GtkOffscreenWindow *)widget);

  if (offscreen_pixbuf != NULL)
  {
    /* skip rendering if the overlay isn't available or visible */
    gboolean is_unavailable = !openvr_overlay_is_valid (overlay) ||
                              !openvr_overlay_is_available (overlay);
    gboolean is_invisible = !openvr_overlay_is_visible (overlay) &&
                            !openvr_overlay_thumbnail_is_visible (overlay);

    if (is_unavailable || is_invisible)
    {
      g_object_unref (offscreen_pixbuf);
      return TRUE;
    }

    GdkPixbuf *pixbuf = gdk_pixbuf_add_alpha (offscreen_pixbuf, false, 0, 0, 0);
    g_object_unref (offscreen_pixbuf);

    openvr_vulkan_client_load_pixbuf (OPENVR_VULKAN_CLIENT (uploader),
                                      texture, pixbuf);
    openvr_vulkan_uploader_submit_frame (uploader, overlay, texture);

    g_object_unref (pixbuf);
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
  openvr_time_substract (&now, &labels->last_time, &diff);

  double diff_s = openvr_time_to_double_secs (&diff);
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

bool
_init_openvr ()
{
  if (!openvr_context_is_installed ())
    {
      g_printerr ("VR Runtime not installed.\n");
      return false;
    }

  OpenVRContext *context = openvr_context_get_instance ();
  if (!openvr_context_init_overlay (context))
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  if (!openvr_context_is_valid (context))
    {
      g_printerr ("Could not load OpenVR function pointers.\n");
      return false;
    }

  return true;
}

int
main (int argc, char *argv[])
{
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

  /* init openvr */
  if (!_init_openvr ())
    return -1;
  OpenVRCompositor *compositor = openvr_compositor_new ();

  uploader = openvr_vulkan_uploader_new ();
  if (!openvr_vulkan_uploader_init_vulkan (uploader, true, compositor))
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return false;
  }

  texture = openvr_vulkan_texture_new ();

  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create_for_dashboard (overlay, "openvr.example.gtk", "GTK+");

  if (!openvr_overlay_is_valid (overlay) ||
      !openvr_overlay_is_available (overlay))
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

  g_object_unref (texture);
  g_object_unref (uploader);

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  return 0;
}
