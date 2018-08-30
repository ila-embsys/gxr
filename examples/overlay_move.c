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
#include <glib/gprintf.h>

#include <openvr-glib.h>

#include "openvr-context.h"
#include "openvr-compositor.h"
#include "openvr-controller.h"
#include "openvr-math.h"
#include "openvr-overlay.h"
#include "openvr-vulkan-uploader.h"

OpenVRVulkanTexture *texture;

GSList *controllers = NULL;

GSList *cat_overlays = NULL;

OpenVROverlay *pointer;
// OpenVROverlay *intersection;

GMainLoop *loop;

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

static void
_register_controller_events (gpointer controller, gpointer unused);

gboolean
_overlay_event_cb (gpointer unused)
{
  (void) unused;
  // TODO: Controllers should be registered in the system event callback
  if (controllers == NULL)
    {
      controllers = openvr_controller_enumerate ();
      g_slist_foreach (controllers, _register_controller_events, NULL);
    }

  g_slist_foreach (controllers, _controller_poll, NULL);

  // openvr_overlay_poll_event (overlay);
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

#if 0
void
_update_intersection_position (OpenVROverlay *overlay,
                               OpenVRIntersectionEvent *event)
{
  graphene_matrix_t transform;
  graphene_matrix_init_from_matrix (&transform, &event->transform);
  openvr_math_matrix_set_translation (&transform, &event->intersection_point);
  openvr_overlay_set_transform_absolute (overlay, &transform);
  openvr_overlay_show (overlay);
}

static void
_move_3d_cb (OpenVROverlay           *overlay,
             OpenVRIntersectionEvent *event,
             gpointer                 data)
{
  (void) overlay;
  (void) data;

  // if we have an intersection point, move the pointer overlay there
  if (event->has_intersection)
    _update_intersection_position (intersection, event);
  else
    openvr_overlay_hide (intersection);

  graphene_matrix_t scale_matrix;
  graphene_matrix_init_scale (&scale_matrix, 1.0f, 1.0f, 5.0f);

  graphene_matrix_t scaled;
  graphene_matrix_multiply (&scale_matrix, &event->transform, &scaled);

  openvr_overlay_set_transform_absolute (pointer, &scaled);

  // TODO: because this is a custom event with a struct that has been allocated
  // specifically for us, we need to free it. Maybe reuse?
  free (event);
}
#endif

static void
_motion_3d_cb (OpenVRController    *controller,
               OpenVRMotion3DEvent *event,
               gpointer             data)
{
  (void) controller;
  (void) data;

  /*
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
  */

  graphene_matrix_t scale_matrix;
  graphene_matrix_init_scale (&scale_matrix, 1.0f, 1.0f, 5.0f);

  graphene_matrix_t scaled;
  graphene_matrix_multiply (&scale_matrix, &event->transform, &scaled);

  openvr_overlay_set_transform_absolute (pointer, &scaled);

  free (event);
}

static void
_press_cb (OpenVRController *controller, GdkEventButton *event, gpointer data)
{
  (void) controller;
  (void) data;
  g_print ("press: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
}

static void
_release_cb (OpenVRController *controller, GdkEventButton *event, gpointer data)
{
  (void) controller;
  (void) data;
  g_print ("release: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
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

  openvr_overlay_create_width (overlay, "pointer", "Pointer", 0.01f);

  if (!openvr_overlay_is_valid (overlay))
    {
      g_printerr ("Overlay unavailable.\n");
      return NULL;
    }

  // TODO: wrap the uint32 of the sort order in some sort of hierarchy
  // for now: The pointer itself should *always* be visible on top of overlays,
  // so use the max value here
  openvr_overlay_set_sort_order (overlay, UINT32_MAX);

  GdkPixbuf *pixbuf = _create_empty_pixbuf (10, 10);
  if (pixbuf == NULL)
    return NULL;

  openvr_overlay_set_gdk_pixbuf_raw (overlay, pixbuf);
  g_object_unref (pixbuf);

  struct HmdColor_t color = {
    .r = 1.0f,
    .g = 1.0f,
    .b = 1.0f,
    .a = 1.0f
  };

  if (!openvr_overlay_set_model (overlay, "{system}laser_pointer", &color))
    return NULL;

  char name_ret[k_unMaxPropertyStringSize];
  struct HmdColor_t color_ret = {};

  uint32_t id;
  if (!openvr_overlay_get_model (overlay, name_ret, &color_ret, &id))
    return NULL;

  g_print ("GetOverlayRenderModel returned id %d\n", id);
  g_print ("GetOverlayRenderModel name %s\n", name_ret);

  if (!openvr_overlay_set_width_meters (overlay, 0.01f))
    return NULL;

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

gboolean
_init_cat_overlay (OpenVRVulkanUploader *uploader)
{
  GdkPixbuf *pixbuf = load_gdk_pixbuf ("/res/cat.jpg");
  if (pixbuf == NULL)
    return -1;

  texture = openvr_vulkan_texture_new ();
  openvr_vulkan_client_load_pixbuf (OPENVR_VULKAN_CLIENT (uploader), texture,
                                    pixbuf);

  float x = -1;

  for (uint32_t i = 0; i < 9; i++)
    {
      OpenVROverlay *cat;
      cat = openvr_overlay_new ();

      char overlay_name[10];
      g_sprintf (overlay_name, "cat%d", i);

      openvr_overlay_create (cat, overlay_name, "A Cat");

      if (!openvr_overlay_is_valid (cat))
        {
          fprintf (stderr, "Overlay unavailable.\n");
          return -1;
        }

      graphene_point3d_t position = {
        .x = x,
        .y = 1,
        .z = -3
      };

      graphene_matrix_t transform;
      graphene_matrix_init_translate (&transform, &position);
      openvr_overlay_set_transform_absolute (cat, &transform);

      openvr_overlay_set_mouse_scale (cat,
                                      (float) gdk_pixbuf_get_width (pixbuf),
                                      (float) gdk_pixbuf_get_height (pixbuf));

      openvr_vulkan_uploader_submit_frame (uploader, cat, texture);

      cat_overlays = g_slist_append (cat_overlays, cat);

      if (!openvr_overlay_show (cat))
        return -1;

      x += 0.5;
    }

  g_object_unref (pixbuf);

  /* connect glib callbacks */
#if 0
  g_signal_connect (cat, "intersection-event", (GCallback)_move_3d_cb,
                    NULL);
  g_signal_connect (cat, "button-press-event", (GCallback)_press_cb, loop);
  g_signal_connect (cat, "scroll-event", (GCallback)_scroll_cb, loop);
  g_signal_connect (cat, "button-release-event", (GCallback)_release_cb,
                    NULL);
  g_signal_connect (cat, "destroy", (GCallback)_destroy_cb, loop);
#endif

  return TRUE;
}

#if 0
gboolean
_init_intersection_overlay (OpenVRVulkanUploader *uploader)
{
  GdkPixbuf *pixbuf = load_gdk_pixbuf ("/res/crosshair.png");
  if (pixbuf == NULL)
    return FALSE;

  OpenVRVulkanTexture *intersection_texture = openvr_vulkan_texture_new ();
  openvr_vulkan_client_load_pixbuf (OPENVR_VULKAN_CLIENT (uploader),
                                    intersection_texture, pixbuf);
  g_object_unref (pixbuf);

  intersection = openvr_overlay_new ();
  openvr_overlay_create_width (intersection, "pointer.intersection",
                               "Interection", 0.15);

  if (!openvr_overlay_is_valid (intersection))
    {
      g_printerr ("Overlay unavailable.\n");
      return FALSE;
    }

  // for now: The crosshair should always be visible, except the pointer can
  // occlude it. The pointer has max sort order, so the crosshair gets max -1
  openvr_overlay_set_sort_order (intersection, UINT32_MAX - 1);

  openvr_vulkan_uploader_submit_frame (uploader,
                                       intersection, intersection_texture);

  return TRUE;
}
#endif

static void
_register_controller_events (gpointer controller, gpointer unused)
{
  (void) unused;
  OpenVRController* c = (OpenVRController*) controller;
  g_signal_connect (c, "motion-3d-event", (GCallback) _motion_3d_cb, NULL);
  g_signal_connect (c, "button-press-event", (GCallback) _press_cb, NULL);
  g_signal_connect (c, "button-release-event", (GCallback) _release_cb, NULL);
}

int
main ()
{
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

  pointer = _init_pointer_overlay ();

  //if (!_init_intersection_overlay (uploader))
  //  return -1;

  if (!_init_cat_overlay (uploader))
    return -1;

  g_timeout_add (20, _overlay_event_cb, NULL);

  signal (SIGINT, _sigint_cb);

  /* start glib main loop */
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_print ("bye\n");

  g_object_unref (pointer);
  // g_object_unref (intersection);
  g_object_unref (texture);
  g_object_unref (uploader);

  g_slist_free_full (controllers, g_object_unref);
  g_slist_free_full (cat_overlays, g_object_unref);

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  return 0;
}
