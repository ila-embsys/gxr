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

#include <openvr-glib.h>

#include "openvr-context.h"
#include "openvr-compositor.h"
#include "openvr-controller.h"
#include "openvr-math.h"
#include "openvr-overlay.h"
#include "openvr-vulkan-uploader.h"


#define NUM_CONTROLLERS 2

OpenVRController *controllers[NUM_CONTROLLERS];
OpenVROverlay *pointer;

gboolean
_overlay_event_cb (gpointer data)
{
  OpenVROverlay *overlay = (OpenVROverlay*) data;

  for (int i = 0; i < NUM_CONTROLLERS; i++)
    openvr_controller_poll_event (controllers[i], overlay);

  openvr_overlay_poll_event (overlay);
  return TRUE;
}

static void
_move_3d_cb (OpenVROverlay           *overlay,
             struct _motion_event_3d *event,
             gpointer                 data)
{
  (void) overlay;
  (void) data;

  openvr_overlay_set_transform_absolute (pointer, &event->transform);

  free (event);
}

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

OpenVROverlay *
_init_pointer_overlay ()
{
  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create_width (overlay, "pointer", "Pointer", 0.5f);

  if (!openvr_overlay_is_valid (overlay))
    {
      g_printerr ("Overlay unavailable.\n");
      return NULL;
    }

  GdkPixbuf *pixbuf = _create_empty_pixbuf (10, 10);
  if (pixbuf == NULL)
    return NULL;

  /* Overlay needs a texture to be set to show model
   * See https://github.com/ValveSoftware/openvr/issues/496
   */
  openvr_overlay_set_gdk_pixbuf_raw (overlay, pixbuf);
  g_object_unref (pixbuf);

  struct HmdColor_t color = {
    .r = 1.0f,
    .g = 1.0f,
    .b = 1.0f,
    .a = 1.0f
  };

  if (!openvr_overlay_set_model (overlay, "dk2_hmd", &color))
    return NULL;

  char name_ret[k_unMaxPropertyStringSize];
  struct HmdColor_t color_ret = {};

  uint32_t id;
  if (!openvr_overlay_get_model (overlay, name_ret, &color_ret, &id))
    return NULL;

  g_print ("GetOverlayRenderModel returned id %d name: %s\n", id, name_ret);

  if (!openvr_overlay_set_alpha (overlay, 0.0f))
    return NULL;

  if (!openvr_overlay_show (overlay))
    return NULL;

  return overlay;
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
main ()
{
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);

  /* init openvr */
  if (!_init_openvr ())
    return -1;

  OpenVRContext *context = openvr_context_get_instance ();
  openvr_context_list_models (context);

  pointer = _init_pointer_overlay ();

  g_signal_connect (pointer, "motion-notify-event-3d",
                    (GCallback)_move_3d_cb, NULL);

  for (int i = 0; i < NUM_CONTROLLERS; i++)
    {
      controllers[i] = openvr_controller_new ();
      openvr_controller_find_by_id (controllers[i], i);
    }

  g_timeout_add (20, _overlay_event_cb, pointer);

  /* start glib main loop */
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_print ("bye\n");

  g_object_unref (pointer);

  for (int i = 0; i < NUM_CONTROLLERS; i++)
    g_object_unref (controllers[i]);

  g_object_unref (context);

  return 0;
}
