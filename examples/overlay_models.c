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

OpenVRVulkanTexture *texture;

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

GdkPixbuf *
load_gdk_pixbuf (const gchar* name)
{
  GError * error = NULL;
  GdkPixbuf *pixbuf_rgb = gdk_pixbuf_new_from_resource (name, &error);

  if (error != NULL)
    {
      fprintf (stderr, "Unable to read file: %s\n", error->message);
      g_error_free (error);
      return NULL;
    }

  GdkPixbuf *pixbuf = gdk_pixbuf_add_alpha (pixbuf_rgb, false, 0, 0, 0);
  g_object_unref (pixbuf_rgb);
  return pixbuf;
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

OpenVROverlay *
_init_pointer_overlay (OpenVRVulkanUploader *uploader)
{
  GdkPixbuf *pixbuf = load_gdk_pixbuf ("/res/cat.jpg");
  if (pixbuf == NULL)
    return NULL;

  OpenVRVulkanTexture *texture = openvr_vulkan_texture_new ();
  openvr_vulkan_client_load_pixbuf (OPENVR_VULKAN_CLIENT (uploader),
                                    texture, pixbuf);

  g_object_unref (pixbuf);

  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create_width (overlay, "pointer", "Pointer", 0.5f);

  if (!openvr_overlay_is_valid (overlay))
    {
      g_printerr ("Overlay unavailable.\n");
      return NULL;
    }

  OpenVRContext *context = openvr_context_get_instance ();
  openvr_context_list_models (context);

  openvr_vulkan_uploader_submit_frame (uploader, overlay, texture);

  char *mode_name = "dk2_hmd";

  struct HmdColor_t color = {
    .r = 1.0f,
    .g = 0.5f,
    .b = 1.0f,
    .a = 1.0f
  };

  if (!openvr_overlay_set_model (overlay, mode_name, &color))
    return NULL;

  char name_ret[k_unMaxPropertyStringSize];
  struct HmdColor_t color_ret = {};

  uint32_t id;
  if (!openvr_overlay_get_model (overlay, name_ret, &color_ret, &id))
    return NULL;

  g_print ("GetOverlayRenderModel returned id %d\n", id);
  g_print ("GetOverlayRenderModel name %s\n", name_ret);

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

  OpenVRVulkanUploader *uploader = openvr_vulkan_uploader_new ();
  if (!openvr_vulkan_uploader_init_vulkan (uploader, true))
    {
      g_printerr ("Unable to initialize Vulkan!\n");
      return false;
    }

  pointer = _init_pointer_overlay (uploader);

  g_signal_connect (pointer, "motion-notify-event-3d", (GCallback)_move_3d_cb,
                    NULL);

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
  g_object_unref (texture);
  g_object_unref (uploader);

  for (int i = 0; i < NUM_CONTROLLERS; i++)
    g_object_unref (controllers[i]);

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  return 0;
}
