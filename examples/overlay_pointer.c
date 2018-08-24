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

OpenVRVulkanTexture *texture;

OpenVRController left;
OpenVRController right;

OpenVROverlay *pointer;

gboolean
_overlay_event_cb (gpointer data)
{
  OpenVROverlay *overlay = (OpenVROverlay*) data;

  if (left.initialized)
    openvr_controller_trigger_events (&left, overlay);
  else
    openvr_controller_init (&left, 0);

  if (right.initialized)
    openvr_controller_trigger_events (&right, overlay);
  else
    openvr_controller_init (&right, 1);

  openvr_overlay_poll_event (overlay);
  return TRUE;
}

GdkPixbuf *
load_gdk_pixbuf ()
{
  GError *   error = NULL;
  GdkPixbuf *pixbuf_unflipped =
      gdk_pixbuf_new_from_resource ("/res/cat.jpg", &error);

  if (error != NULL)
    {
      fprintf (stderr, "Unable to read file: %s\n", error->message);
      g_error_free (error);
      return NULL;
    }
  else
    {
      // GdkPixbuf * pixbuf = gdk_pixbuf_flip (pixbuf_unflipped, FALSE);
      GdkPixbuf *pixbuf =
          gdk_pixbuf_add_alpha (pixbuf_unflipped, false, 0, 0, 0);
      g_object_unref (pixbuf_unflipped);
      // g_object_unref (pixbuf);
      return pixbuf;
    }
}

static void
_move_3d_cb (OpenVROverlay           *overlay,
             struct _motion_event_3d *event,
             gpointer                 data)
{
  (void) overlay;
  (void) data;

  /*
  HmdMatrix34_t translation34;
  openvr_math_graphene_to_matrix34 (&event->transform, &translation34);

  // if we have an intersection point, move the pointer overlay there
  if (event->has_intersection)
    {
      translation34.m[0][3] = event->intersection_point.x;
      translation34.m[1][3] = event->intersection_point.y;
      translation34.m[2][3] = event->intersection_point.z;
      // g_print("%f %f %f\n", translation34.m[0][3], translation34.m[1][3],
      // translation34.m[2][3]);
    }

  OpenVRContext *context = openvr_context_get_instance ();
  int err = context->overlay->SetOverlayTransformAbsolute (
      pointer->overlay_handle, context->origin, &translation34);
  if (err != EVROverlayError_VROverlayError_None)
    {
      g_print("Failed to set overlay transform!\n");
    }
  */
  openvr_overlay_set_transform_absolute (pointer, &event->transform);

  // TODO: because this is a custom event with a struct that has been allocated
  // specifically for us, we need to free it. Maybe reuse?
  free (event);
}

static void
_scroll_cb (OpenVROverlay *overlay, GdkEventScroll *event, gpointer data)
{
  (void) overlay;
  (void) data;
  g_print ("scroll: %s %f\n",
           event->direction == GDK_SCROLL_UP ? "up" : "down", event->y);
}

static void
_press_cb (OpenVROverlay *overlay, GdkEventButton *event, gpointer data)
{
  (void) overlay;
  (void) data;
  g_print ("press: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
}

static void
_release_cb (OpenVROverlay *overlay, GdkEventButton *event, gpointer data)
{
  (void) overlay;
  (void) data;
  g_print ("release: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
}

static void
_destroy_cb (OpenVROverlay *overlay, gpointer data)
{
  (void) overlay;
  g_print ("destroy\n");
  GMainLoop *loop = (GMainLoop *)data;
  g_main_loop_quit (loop);
}

OpenVROverlay *
_init_pointer_overlay (OpenVRVulkanUploader *uploader)
{
  GdkPixbuf *pixbuf = load_gdk_pixbuf ();
  if (pixbuf == NULL)
    return NULL;

  OpenVRVulkanTexture *texture = openvr_vulkan_texture_new ();
  openvr_vulkan_client_load_pixbuf (OPENVR_VULKAN_CLIENT (uploader),
                                    texture, pixbuf);

  g_object_unref (pixbuf);

  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create_width (overlay, "pointer", "Pointer", 0.1f);

  if (!openvr_overlay_is_valid (overlay))
    {
      g_printerr ("Overlay unavailable.\n");
      return NULL;
    }

  OpenVRContext *context = openvr_context_get_instance ();
  openvr_context_list_models (context);

  openvr_vulkan_uploader_submit_frame (uploader, overlay, texture);

  char *mode_name = "{system}laser_pointer";

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
test_cat_overlay ()
{
  GMainLoop *loop;

  GdkPixbuf *pixbuf = load_gdk_pixbuf ();
  if (pixbuf == NULL)
    return -1;

  loop = g_main_loop_new (NULL, FALSE);

  /* init openvr */
  if (!_init_openvr ())
    return -1;

  OpenVRVulkanUploader *uploader = openvr_vulkan_uploader_new ();
  if (!openvr_vulkan_uploader_init_vulkan (uploader, true))
    {
      g_printerr ("Unable to initialize Vulkan!\n");
      return false;
    }

  texture = openvr_vulkan_texture_new ();

  openvr_vulkan_client_load_pixbuf (OPENVR_VULKAN_CLIENT (uploader), texture,
                                    pixbuf);

  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create (overlay, "vulkan.cat", "Vulkan Cat");

  if (!openvr_overlay_is_valid (overlay))
    {
      fprintf (stderr, "Overlay unavailable.\n");
      return -1;
    }

  OpenVROverlay *overlay2 = openvr_overlay_new ();
  openvr_overlay_create (overlay2, "vulkan.cat2", "Another Vulkan Cat");

  if (!openvr_overlay_is_valid (overlay2))
    {
      fprintf (stderr, "Overlay 2 unavailable.\n");
      return -1;
    }

  openvr_overlay_set_mouse_scale (overlay,
                                  (float)gdk_pixbuf_get_width (pixbuf),
                                  (float)gdk_pixbuf_get_height (pixbuf));

  if (!openvr_overlay_show (overlay))
    return -1;

  pointer = _init_pointer_overlay (uploader);

  openvr_vulkan_uploader_submit_frame (uploader, overlay, texture);
  openvr_vulkan_uploader_submit_frame (uploader, overlay2, texture);

  /* connect glib callbacks */
  // g_signal_connect (overlay, "motion-notify-event", (GCallback)_move_cb, NULL);
  g_signal_connect (overlay, "motion-notify-event-3d", (GCallback)_move_3d_cb,
                    NULL);
  g_signal_connect (overlay, "button-press-event", (GCallback)_press_cb, loop);
  g_signal_connect (overlay, "scroll-event", (GCallback)_scroll_cb, loop);
  g_signal_connect (overlay, "button-release-event", (GCallback)_release_cb,
                    NULL);
  g_signal_connect (overlay, "destroy", (GCallback)_destroy_cb, loop);

  g_timeout_add (20, _overlay_event_cb, overlay);

  /* start glib main loop */
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_print ("bye\n");

  g_object_unref (overlay);
  g_object_unref (pixbuf);
  g_object_unref (texture);
  g_object_unref (uploader);

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  return 0;
}

int
main ()
{
  left.initialized = FALSE;
  right.initialized = FALSE;
  return test_cat_overlay ();
}
