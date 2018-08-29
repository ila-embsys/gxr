/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <graphene.h>
#include <signal.h>

#include <openvr-glib.h>

#include "openvr-context.h"
#include "openvr-compositor.h"
#include "openvr-controller.h"
#include "openvr-math.h"
#include "openvr-overlay.h"
#include "openvr-vulkan-uploader.h"


GSList *controllers;
OpenVROverlay *pointer;
GMainLoop *loop;
guint current_model_list_index = 0;
GSList *models = NULL;

void
_sigint_cb (int sig_num)
{
  if (sig_num == SIGINT)
    g_main_loop_quit (loop);
}

static void
_controller_poll (gpointer controller, gpointer unused)
{
  (void) unused;
  openvr_controller_poll_event ((OpenVRController*) controller);
}

gboolean
_poll_cb (gpointer nothing)
{
  (void) nothing;
  g_slist_foreach (controllers, _controller_poll, NULL);

  return TRUE;
}

static void
_motion_3d_cb (OpenVRController    *controller,
               OpenVRMotion3DEvent *event,
               gpointer             data)
{
  (void) controller;
  (void) data;

  graphene_point3d_t translation_point;

  graphene_point3d_init (&translation_point, .0f, .1f, -.1f);

  graphene_matrix_t transformation_matrix;
  graphene_matrix_init_translate (&transformation_matrix, &translation_point);

  graphene_matrix_scale (&transformation_matrix, 1.0f, 1.0f, 0.5f);

  graphene_matrix_t transformed;
  graphene_matrix_multiply (&transformation_matrix,
                            &event->transform,
                            &transformed);

  openvr_overlay_set_transform_absolute (pointer, &transformed);

  free (event);
}

gboolean
_update_model ()
{
  struct HmdColor_t color = {
    .r = 1.0f,
    .g = 1.0f,
    .b = 1.0f,
    .a = 1.0f
  };

  GSList* name = g_slist_nth (models, current_model_list_index);
  g_print ("Setting Model '%s' [%d/%d]\n",
           (gchar *) name->data,
           current_model_list_index + 1,
           g_slist_length (models));

  if (!openvr_overlay_set_model (pointer, (gchar *) name->data, &color))
    return FALSE;

  return TRUE;
}

gboolean
_next_model ()
{
  if (current_model_list_index == g_slist_length (models) - 1)
    current_model_list_index = 0;
  else
    current_model_list_index++;

  if (!_update_model ())
    return FALSE;

  return TRUE;
}

gboolean
_previous_model ()
{
  if (current_model_list_index == 0)
    current_model_list_index = g_slist_length (models) - 1;
  else
    current_model_list_index--;

  if (!_update_model ())
    return FALSE;

  return TRUE;
}

static void
_press_cb (OpenVRController *controller, GdkEventButton *event, gpointer data)
{
  (void) controller;
  (void) data;

  switch (event->button)
    {
      case OPENVR_BUTTON_AXIS0:
        {
          if (event->x > 0)
            _next_model ();
          else
            _previous_model ();
        }
    }
}

#if 0
static void
_release_cb (OpenVRController *controller, GdkEventButton *event, gpointer data)
{
  (void) controller;
  (void) data;
  // g_print ("release: %d %f %f (%d)\n",
  //          event->button, event->x, event->y, event->time);

  switch (event->button)
    {
      case OPENVR_BUTTON_AXIS0:
        {
          // g_print ("Touchpad release: [%f %f]\n", event->x, event->y);
          if (event->x > 0)
            g_print ("release right\n");
          else
            g_print ("release left\n");
        }
    }
}

static void
_pad_motion_cb (OpenVRController *controller,
                GdkEventMotion   *event,
                gpointer          data)
{
  (void) controller;
  (void) data;
  g_print ("pad motion: %f %f (%d)\n", event->x, event->y, event->time);
}
#endif

GdkPixbuf *
_create_empty_pixbuf (uint32_t width, uint32_t height)
{
  guchar pixels[height * width * 4];
  memset (pixels, 0, height * width * 4 * sizeof (guchar));
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB,
                                                TRUE, 8, width, height,
                                                4 * width, NULL, NULL);
  return pixbuf;
}

gboolean
_init_pointer_overlay ()
{
  pointer = openvr_overlay_new ();
  openvr_overlay_create_width (pointer, "pointer", "Pointer", 0.5f);

  if (!openvr_overlay_is_valid (pointer))
    {
      g_printerr ("Overlay unavailable.\n");
      return FALSE;
    }

  GdkPixbuf *pixbuf = _create_empty_pixbuf (10, 10);
  if (pixbuf == NULL)
    return FALSE;

  /* Overlay needs a texture to be set to show model
   * See https://github.com/ValveSoftware/openvr/issues/496
   */
  openvr_overlay_set_gdk_pixbuf_raw (pointer, pixbuf);
  g_object_unref (pixbuf);

  struct HmdColor_t color = {
    .r = 1.0f,
    .g = 1.0f,
    .b = 1.0f,
    .a = 1.0f
  };

  GSList* model_name = g_slist_nth (models, current_model_list_index);
  if (!openvr_overlay_set_model (pointer, (gchar *) model_name->data, &color))
    return FALSE;

  char name_ret[k_unMaxPropertyStringSize];
  struct HmdColor_t color_ret = {};

  uint32_t id;
  if (!openvr_overlay_get_model (pointer, name_ret, &color_ret, &id))
    return FALSE;

  g_print ("GetOverlayRenderModel returned id %d name: %s\n", id, name_ret);

  if (!openvr_overlay_set_alpha (pointer, 0.0f))
    return FALSE;

  if (!openvr_overlay_set_width_meters (pointer, 0.5f))
    return FALSE;

  if (!openvr_overlay_show (pointer))
    return FALSE;

  return TRUE;
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

static void
_register_controller_events (gpointer controller, gpointer unused)
{
  (void) unused;
  OpenVRController* c = (OpenVRController*) controller;
  g_signal_connect (c, "motion-3d-event", (GCallback) _motion_3d_cb, NULL);
  g_signal_connect (c, "button-press-event", (GCallback) _press_cb, NULL);
  // g_signal_connect (c, "button-release-event", (GCallback) _release_cb, NULL);
  // g_signal_connect (c, "touchpad-motion-event",
  //                   (GCallback) _pad_motion_cb, NULL);
}

static void
_print_model (gpointer name, gpointer unused)
{
  (void) unused;
  g_print ("Model: %s\n", (gchar*) name);
}

int
main ()
{
  loop = g_main_loop_new (NULL, FALSE);

  /* init openvr */
  if (!_init_openvr ())
    return -1;

  OpenVRContext *context = openvr_context_get_instance ();

  // openvr_context_list_models (context);
  models = openvr_context_get_model_list (context);
  g_slist_foreach (models, _print_model, NULL);

  if (!_init_pointer_overlay ())
    return -1;

  controllers = openvr_controller_enumerate ();

  g_slist_foreach (controllers, _register_controller_events, NULL);

  g_timeout_add (20, _poll_cb, NULL);

  signal (SIGINT, _sigint_cb);

  /* start glib main loop */
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_print ("bye\n");

  g_object_unref (pointer);

  g_slist_free_full (controllers, g_object_unref);
  g_slist_free_full (models, g_free);

  g_object_unref (context);

  return 0;
}
