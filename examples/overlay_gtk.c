/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <time.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "gxr.h"

static GulkanTexture *texture = NULL;
static GulkanClient *uploader;

static gboolean
_damage_cb (GtkWidget *widget, GdkEventExpose *event, OpenVROverlay *overlay)
{
  (void) event;
  GdkPixbuf * offscreen_pixbuf =
    gtk_offscreen_window_get_pixbuf ((GtkOffscreenWindow *)widget);

  if (offscreen_pixbuf != NULL)
  {
    /* skip rendering if the overlay isn't available or visible */
    gboolean is_invisible = !openvr_overlay_is_visible (overlay) &&
                            !openvr_overlay_thumbnail_is_visible (overlay);

    if (!openvr_overlay_is_valid (overlay) || is_invisible)
      {
        g_object_unref (offscreen_pixbuf);
        return TRUE;
      }

    GdkPixbuf *pixbuf = gdk_pixbuf_add_alpha (offscreen_pixbuf, false, 0, 0, 0);
    g_object_unref (offscreen_pixbuf);

    GulkanClient *client = GULKAN_CLIENT (uploader);

    if (texture == NULL)
      texture = gulkan_client_texture_new_from_pixbuf (client, pixbuf,
                                                       VK_FORMAT_R8G8B8A8_UNORM,
                                                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                       true);

    openvr_overlay_submit_texture (overlay, uploader, texture);

    g_object_unref (pixbuf);
  } else {
    fprintf (stderr, "Could not acquire pixbuf.\n");
  }

  return TRUE;
}

static gboolean
_button_press_cb (GtkWidget *button, GdkEventButton *event, gpointer data)
{
  (void) button;
  (void) event;
  (void) data;
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
  (void) widget;
  (void) cr;
  struct timespec now;
  if (clock_gettime (CLOCK_REALTIME, &now) != 0)
  {
    fprintf (stderr, "Could not read system clock\n");
    return 0;
  }

  struct timespec diff;
  gxr_time_substract (&now, &labels->last_time, &diff);

  double diff_s = gxr_time_to_double_secs (&diff);
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

static gboolean
timeout_callback (gpointer data)
{
  OpenVROverlay *overlay = (OpenVROverlay*) data;
  openvr_overlay_poll_event (overlay);
  return TRUE;
}

static void
_press_cb (OpenVROverlay  *overlay,
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
_destroy_cb (OpenVROverlay *overlay,
             gpointer       data)
{
  (void) overlay;
  g_print ("destroy\n");
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}

static bool
_init_openvr (GxrContext *context, GulkanClient *client)
{
  if (!gxr_context_init_runtime (context, GXR_APP_OVERLAY))
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  if (!gxr_context_init_gulkan (context, client))
    {
      g_printerr ("Unable to initialize Vulkan!\n");
      return false;
    }

  if (!gxr_context_init_session (context, client))
    {
      g_printerr ("Could not init OpenVR session.\n");
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
  gtk_box_pack_start (GTK_BOX (box), labels.time_label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), labels.fps_label, FALSE, TRUE, 0);

  gtk_widget_set_size_request (window , 500, 300);
  gtk_container_add (GTK_CONTAINER (window), box);

  gtk_widget_show_all (window);

  GxrContext *context = gxr_context_get_instance ();
  GulkanClient *uploader = gulkan_client_new ();

  /* init openvr */
  if (!_init_openvr (context, uploader))
    return -1;

  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create_width (overlay, "openvr.example.gtk", "GTK+", 1.0);
  openvr_overlay_show (overlay);
  graphene_point3d_t position = { .x = 0.f, .y = 1.2f, .z = -1.f };
  openvr_overlay_set_translation (overlay, &position);

  if (!openvr_overlay_is_valid (overlay))
  {
    fprintf (stderr, "Overlay unavailable.\n");
    return -1;
  }

  openvr_overlay_set_mouse_scale (overlay, 300.0f, 200.0f);

  g_signal_connect (overlay, "button-press-event", (GCallback) _press_cb, loop);
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

  g_object_unref (context);

  return 0;
}
