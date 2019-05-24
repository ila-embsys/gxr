/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#define GETTEXT_PACKAGE "gtk30"

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gprintf.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "openvr-glib.h"

static gboolean use_system_keyboard = FALSE;

typedef struct Example
{
  int size_x;
  int size_y;

  int text_cursor;
  char input_text[300];
  GulkanTexture *texture;
  GulkanClient *uploader;
  OpenVROverlay *overlay;

  OpenVRActionSet *action_set;

  GtkWidget *label;

  GMainLoop *loop;

} Example;


static gboolean
_damage_cb (GtkWidget      *widget,
            GdkEventExpose *event,
            gpointer       _self)
{
  (void) event;

  Example *self = (Example*) _self;

  GdkPixbuf * offscreen_pixbuf =
    gtk_offscreen_window_get_pixbuf ((GtkOffscreenWindow *)widget);

  if (offscreen_pixbuf != NULL)
  {
    GdkPixbuf *pixbuf = gdk_pixbuf_add_alpha (offscreen_pixbuf, false, 0, 0, 0);
    g_object_unref (offscreen_pixbuf);

    GulkanClient *client = GULKAN_CLIENT (self->uploader);

    if (self->texture == NULL)
      self->texture = gulkan_client_texture_new_from_pixbuf (client, pixbuf,
                                                             VK_FORMAT_R8G8B8A8_UNORM,
                                                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                             false);
    else
      gulkan_client_upload_pixbuf (client, self->texture, pixbuf,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    openvr_overlay_submit_texture (self->overlay, self->uploader,
                                   self->texture);

    g_object_unref (pixbuf);
  } else {
    g_printerr ("Could not acquire pixbuf.\n");
  }

  return TRUE;
}

static void
_destroy_cb (OpenVROverlay *overlay,
             gpointer      _self)
{
  (void) overlay;
  g_print ("destroy\n");
  Example *self = (Example*) _self;
  g_main_loop_quit (self->loop);
}

void
_process_key_event (Example *self, GdkEventKey *event)
{
  g_print ("Input str %s (%d)\n", event->string, event->length);
  for (int i = 0; i < event->length; i++)
    {
      // 8 is backspace
      if (event->string[i] == 8 && self->text_cursor > 0)
        self->input_text[self->text_cursor--] = 0;
      else if (self->text_cursor < 300)
        self->input_text[self->text_cursor++] = event->string[i];
    }

  gchar markup_str [50];
  g_sprintf (markup_str, "<span font=\"24\">%s</span>", self->input_text);
  gtk_label_set_markup (GTK_LABEL (self->label), markup_str);
}

static void
_system_keyboard_press_cb (OpenVRContext *context,
                           GdkEventKey   *event,
                           gpointer      _self)
{
  (void) context;
  _process_key_event ((Example*) _self, event);
}

static void
_overlay_keyboard_press_cb (OpenVROverlay *overlay,
                            GdkEventKey   *event,
                            gpointer      _self)
{
  (void) overlay;
  _process_key_event ((Example*) _self, event);
}

static void
_system_keyboard_close_cb (OpenVRContext  *context,
                           gpointer       _self)
{
  (void) context;
  (void) _self;
  g_print ("System keyboard closed.\n");
}

static void
_overlay_keyboard_close_cb (OpenVROverlay  *overlay,
                            gpointer       _self)
{
  (void) overlay;
  (void) _self;
  g_print ("Overlay keyboard closed.\n");
}

gboolean
_poll_events_cb (gpointer _self)
{
  Example *self = (Example*) _self;

  openvr_action_set_poll (self->action_set);
  openvr_overlay_poll_event (self->overlay);

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
  Example *self = (Example*) _self;

  if (event->state && event->changed)
    {
      if (use_system_keyboard)
        {
          OpenVRContext *context = openvr_context_get_instance ();
          openvr_context_show_system_keyboard (context);
        }
      else
        {
          openvr_overlay_show_keyboard (self->overlay);
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
_init_gtk (Example *self)
{
  GtkWidget *window = gtk_offscreen_window_new ();

  self->label = gtk_label_new ("");

  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  gtk_box_pack_start (GTK_BOX (box), self->label, TRUE, FALSE, 0);

  gtk_widget_set_size_request (window , self->size_x, self->size_y);
  gtk_container_add (GTK_CONTAINER (window), box);

  gtk_widget_show_all (window);

  g_signal_connect (window, "damage-event", G_CALLBACK (_damage_cb), self);

  return TRUE;
}

gboolean
_create_overlay (Example *self)
{
  self->overlay = openvr_overlay_new ();
  openvr_overlay_create_width (self->overlay,
                               "openvr.example.keyboard",
                               "Keyboard Test", 5.0);

  if (!openvr_overlay_is_valid (self->overlay))
  {
    g_printerr ("Overlay unavailable.\n");
    return FALSE;
  }

  if (!openvr_overlay_show (self->overlay))
    return FALSE;

  graphene_point3d_t initial_position = {
    .x = 0,
    .y = 1,
    .z = -2.5
  };
  graphene_matrix_t transform;
  graphene_matrix_init_translate (&transform, &initial_position);
  openvr_overlay_set_transform_absolute (self->overlay, &transform);

  g_signal_connect (self->overlay, "destroy", (GCallback) _destroy_cb, self);

  return TRUE;
}

void
_cleanup (Example *self)
{
  g_main_loop_unref (self->loop);
  g_object_unref (self->overlay);
  g_object_unref (self->texture);
  g_object_unref (self->uploader);

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);
}

int
main (int argc, char *argv[])
{
  if (!_parse_options (&argc, &argv))
    return -1;

  gtk_init (&argc, &argv);

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

  Example self = {
    .loop = g_main_loop_new (NULL, FALSE),
    .size_x = 800,
    .size_y = 600,
    .text_cursor = 0,
    .texture = NULL,
    .uploader = openvr_compositor_gulkan_client_new (true),
    .action_set = openvr_action_set_new_from_url ("/actions/wm")
  };

  if (!_init_gtk (&self))
    return FALSE;

  if (!self.uploader)
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return false;
  }

  if (!_create_overlay (&self))
    return -1;

  openvr_action_set_connect (self.action_set, OPENVR_ACTION_DIGITAL,
                             "/actions/wm/in/show_keyboard",
                             (GCallback) _show_keyboard_cb, &self);

  if (use_system_keyboard)
    {
      g_signal_connect (context, "keyboard-press-event",
                        (GCallback) _system_keyboard_press_cb, &self);
      g_signal_connect (context, "keyboard-close-event",
                        (GCallback) _system_keyboard_close_cb, &self);
    }
  else
    {
      g_signal_connect (self.overlay, "keyboard-press-event",
                        (GCallback) _overlay_keyboard_press_cb, &self);
      g_signal_connect (self.overlay, "keyboard-close-event",
                        (GCallback) _overlay_keyboard_close_cb, &self);
    }

  g_timeout_add (20, _poll_events_cb, &self);

  g_main_loop_run (self.loop);

  _cleanup (&self);

  return 0;
}
