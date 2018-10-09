/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <time.h>
#include <glib.h>


#define GETTEXT_PACKAGE "gtk30"
#include <glib/gi18n-lib.h>

#include <glib/gprintf.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <openvr-glib.h>

#include "openvr-context.h"
#include "openvr-overlay.h"
#include "openvr-time.h"
#include "openvr-vulkan-uploader.h"

#include "openvr-io.h"
#include "openvr-action.h"
#include "openvr-action-set.h"


static gboolean use_system_keyboard = FALSE;

int size_x = 800;
int size_y = 600;

int text_cursor = 0;
char input_text[300];

OpenVRVulkanTexture *texture = NULL;
OpenVRVulkanUploader *uploader;

OpenVROverlay *overlay;

static gboolean
_damage_cb (GtkWidget *widget, GdkEventExpose *event, gpointer unused)
{
  (void) event;
  (void) unused;
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

    OpenVRVulkanClient *client = OPENVR_VULKAN_CLIENT (uploader);

    if (texture == NULL)
      texture = openvr_vulkan_texture_new_from_pixbuf (client->device, pixbuf);

    openvr_vulkan_client_upload_pixbuf (client, texture, pixbuf);

    openvr_vulkan_uploader_submit_frame (uploader, overlay, texture);

    g_object_unref (pixbuf);
  } else {
    g_printerr ("Could not acquire pixbuf.\n");
  }

  return TRUE;
}

typedef struct Labels
{
  GtkWidget *time_label;
} Labels;

static gboolean
_draw_cb (GtkWidget *widget, cairo_t *cr, Labels* labels)
{
  (void) widget;
  (void) cr;

  gchar display_str [50];

  g_sprintf (display_str, "<span font=\"24\">%s</span>", input_text);

  gtk_label_set_markup (GTK_LABEL (labels->time_label), display_str);

  return FALSE;
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

static void
_system_keyboard_cb (OpenVRContext  *context,
                     GdkEventKey    *event,
                     gpointer        data)
{
  (void) context;
  (void) data;

  g_print ("Input str %s (%d)\n", event->string, event->length);
  for (int i = 0; i < event->length; i++)
    {
      // 8 is backspace
      if (event->string[i] == 8 && text_cursor > 0)
        input_text[text_cursor--] = 0;
      else if (text_cursor < 300)
        input_text[text_cursor++] = event->string[i];
    }
}

static void
_overlay_keyboard_cb (OpenVROverlay  *overlay,
                      GdkEventKey    *event,
                      gpointer        data)
{
  (void) data;

  g_print ("Input str %s (%d) for overlay %lu\n", event->string, event->length,
           overlay->overlay_handle);
  for (int i = 0; i < event->length; i++)
    {
      // 8 is backspace
      if (event->string[i] == 8 && text_cursor > 0)
        input_text[text_cursor--] = 0;
      else if (text_cursor < 300)
        input_text[text_cursor++] = event->string[i];
    }
}

gboolean
_poll_events_cb (gpointer data)
{
  OpenVRActionSet *action_set = data;

  openvr_action_set_poll (action_set);

  openvr_overlay_poll_event (overlay);

  OpenVRContext *context = openvr_context_get_instance ();
  openvr_context_poll_event (context);

  return TRUE;
}

static void
_show_keyboard_cb (OpenVRAction       *action,
                   OpenVRDigitalEvent *event,
                   gpointer           _self)
{
  (void) action;
  (void) _self;
  if (event->state && event->changed)
    {
      if (use_system_keyboard)
        {
          OpenVRContext *context = openvr_context_get_instance ();
          openvr_context_show_system_keyboard (context);
        }
      else
        {
          openvr_overlay_show_keyboard (overlay);
          g_signal_connect (overlay, "keyboard-char-input-event",
                            (GCallback) _overlay_keyboard_cb, NULL);
        }
    }
}

static GOptionEntry entries[] =
{
  { "system-keyboard", 0, 0, G_OPTION_ARG_NONE, &use_system_keyboard,
    "Use system keyboard", NULL },
  { NULL }
};

gboolean
_parse_options (gint *argc, gchar ***argv)
{
  GError *error = NULL;
  GOptionContext *option_context;

  option_context = g_option_context_new ("OpenVR Keyboard Example");
  g_option_context_add_main_entries (option_context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (option_context, gtk_get_option_group (TRUE));
  if (!g_option_context_parse (option_context, argc, argv, &error))
    {
      g_print ("option parsing failed: %s\n", error->message);
      return FALSE;
    }
  return TRUE;
}

gboolean
_init_gtk (Labels *labels)
{
  GtkWidget *window = gtk_offscreen_window_new ();

  labels->time_label = gtk_label_new ("");

  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  gtk_box_pack_start (GTK_BOX (box), labels->time_label, TRUE, FALSE, 0);

  gtk_widget_set_size_request (window , size_x, size_y);
  gtk_container_add (GTK_CONTAINER (window), box);

  gtk_widget_show_all (window);

  g_signal_connect (window, "damage-event", G_CALLBACK (_damage_cb), NULL);
  g_signal_connect (window, "draw", G_CALLBACK (_draw_cb), labels);

  return TRUE;
}

gboolean
_create_overlay (GMainLoop *loop)
{
  overlay = openvr_overlay_new ();
  openvr_overlay_create_width (overlay,
                               "openvr.example.keyboard",
                               "Keyboard Test", 5.0);

  if (!openvr_overlay_is_valid (overlay))
  {
    g_printerr ("Overlay unavailable.\n");
    return FALSE;
  }

  if (!openvr_overlay_show (overlay))
    return FALSE;

  graphene_point3d_t initial_position = {
    .x = 0,
    .y = 1,
    .z = -2.5
  };
  graphene_matrix_t transform;
  graphene_matrix_init_translate (&transform, &initial_position);
  openvr_overlay_set_transform_absolute (overlay, &transform);

  g_signal_connect (overlay, "destroy", (GCallback) _destroy_cb, loop);

  return TRUE;
}

int
main (int argc, char *argv[])
{
  if (!_parse_options (&argc, &argv))
    return -1;

  GMainLoop *loop;

  gtk_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);

  Labels labels = {};
  if (!_init_gtk (&labels))
    return FALSE;

  OpenVRContext *context = openvr_context_get_instance ();
  if (!openvr_context_init_overlay (context))
    {
      g_printerr ("Could not init OpenVR.\n");
      return -1;
    }

  if (!openvr_io_load_cached_action_manifest (
      "openvr-glib",
      "/res/bindings",
      "actions.json",
      "bindings_vive_controller.json",
      "bindings_knuckles_controller.json",
      NULL))
    return -1;

  uploader = openvr_vulkan_uploader_new ();
  if (!openvr_vulkan_uploader_init_vulkan (uploader, true))
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return false;
  }

  if (!_create_overlay (loop))
    return -1;

  OpenVRActionSet *action_set =
    openvr_action_set_new_from_url ("/actions/wm");

  openvr_action_set_connect (action_set, OPENVR_ACTION_DIGITAL,
                             "/actions/wm/in/show_keyboard",
                             (GCallback) _show_keyboard_cb, NULL);

  g_signal_connect (context, "keyboard-char-input-event",
                    (GCallback) _system_keyboard_cb, NULL);

  g_timeout_add (20, _poll_events_cb, action_set);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_object_unref (overlay);

  g_object_unref (texture);
  g_object_unref (uploader);

  g_object_unref (context);

  return 0;
}
