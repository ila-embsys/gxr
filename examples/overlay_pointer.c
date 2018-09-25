/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib-unix.h>

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

typedef struct Example
{
  OpenVRVulkanUploader *uploader;
  OpenVRVulkanTexture *texture;

  GSList *controllers;

  OpenVROverlay *pointer;
  OpenVROverlay *intersection;
  OpenVROverlay *cat;

  GMainLoop *loop;
} Example;

gboolean
_sigint_cb (gpointer _self)
{
  Example *self = (Example*) _self;
  g_main_loop_quit (self->loop);
  return TRUE;
}

static void
_controller_poll (gpointer controller, gpointer overlay)
{
  openvr_overlay_poll_controller_event ((OpenVROverlay*) overlay,
                                        (OpenVRController*) controller);
}

gboolean
_overlay_event_cb (gpointer _self)
{
  Example *self = (Example*) _self;
  // TODO: Controllers should be registered in the system event callback
  if (self->controllers == NULL)
    self->controllers = openvr_controller_enumerate ();

  g_slist_foreach (self->controllers, _controller_poll, self->cat);

  openvr_overlay_poll_event (self->cat);
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
_intersection_cb (OpenVROverlay           *overlay,
                  OpenVRIntersectionEvent *event,
                  gpointer                 _self)
{
  (void) overlay;

  Example *self = (Example*) _self;

  // if we have an intersection point, move the pointer overlay there
  if (event->has_intersection)
    _update_intersection_position (self->intersection, event);
  else
    openvr_overlay_hide (self->intersection);

  graphene_matrix_t scale_matrix;
  graphene_matrix_init_scale (&scale_matrix, 1.0f, 1.0f, 5.0f);

  graphene_matrix_t scaled;
  graphene_matrix_multiply (&scale_matrix, &event->transform, &scaled);

  openvr_overlay_set_transform_absolute (self->pointer, &scaled);

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
_init_cat_overlay (Example *self)
{
  GdkPixbuf *pixbuf = load_gdk_pixbuf ("/res/cat.jpg");
  if (pixbuf == NULL)
    return -1;

  self->cat = openvr_overlay_new ();
  openvr_overlay_create (self->cat, "vulkan.cat", "Vulkan Cat");

  if (!openvr_overlay_is_valid (self->cat))
    {
      fprintf (stderr, "Overlay unavailable.\n");
      return -1;
    }

  graphene_point3d_t position = {
    .x = -1,
    .y = 1,
    .z = -3
  };

  graphene_matrix_t transform;
  graphene_matrix_init_translate (&transform, &position);
  openvr_overlay_set_transform_absolute (self->cat, &transform);

  openvr_overlay_set_mouse_scale (self->cat,
                                  (float) gdk_pixbuf_get_width (pixbuf),
                                  (float) gdk_pixbuf_get_height (pixbuf));

  if (!openvr_overlay_show (self->cat))
    return -1;

  self->texture = openvr_vulkan_texture_new ();

  openvr_vulkan_client_load_pixbuf (OPENVR_VULKAN_CLIENT (self->uploader),
                                    self->texture,
                                    pixbuf);

  g_object_unref (pixbuf);

  openvr_vulkan_uploader_submit_frame (self->uploader,
                                       self->cat, self->texture);

  /* connect glib callbacks */
  g_signal_connect (self->cat, "intersection-event",
                    (GCallback)_intersection_cb,
                    self);
  g_signal_connect (self->cat, "button-press-event",
                    (GCallback)_press_cb,
                    self->loop);
  g_signal_connect (self->cat, "scroll-event",
                    (GCallback)_scroll_cb,
                    self->loop);
  g_signal_connect (self->cat, "button-release-event",
                    (GCallback)_release_cb,
                    NULL);
  g_signal_connect (self->cat, "destroy",
                    (GCallback)_destroy_cb,
                    self->loop);

  return TRUE;
}

gboolean
_init_intersection_overlay (Example *self)
{
  GdkPixbuf *pixbuf = load_gdk_pixbuf ("/res/crosshair.png");
  if (pixbuf == NULL)
    return FALSE;

  OpenVRVulkanTexture *intersection_texture = openvr_vulkan_texture_new ();
  openvr_vulkan_client_load_pixbuf (OPENVR_VULKAN_CLIENT (self->uploader),
                                    intersection_texture, pixbuf);
  g_object_unref (pixbuf);

  self->intersection = openvr_overlay_new ();
  openvr_overlay_create_width (self->intersection, "pointer.intersection",
                               "Interection", 0.15);

  if (!openvr_overlay_is_valid (self->intersection))
    {
      g_printerr ("Overlay unavailable.\n");
      return FALSE;
    }

  // for now: The crosshair should always be visible, except the pointer can
  // occlude it. The pointer has max sort order, so the crosshair gets max -1
  openvr_overlay_set_sort_order (self->intersection, UINT32_MAX - 1);

  openvr_vulkan_uploader_submit_frame (self->uploader,
                                       self->intersection,
                                       intersection_texture);

  return TRUE;
}

void
_cleanup (Example *self)
{
  g_print ("bye\n");

  g_object_unref (self->cat);
  g_object_unref (self->pointer);
  g_object_unref (self->intersection);
  g_object_unref (self->texture);

  /* destroy context before uploader because context finalization calls
   * VR_ShutdownInternal() which accesses the vulkan device that is being
   * destroyed by uploader finalization
   */

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  g_object_unref (self->uploader);

  g_slist_free_full (self->controllers, g_object_unref);
}

int
main ()
{
  Example self = {
    .loop = g_main_loop_new (NULL, FALSE),
    .controllers = NULL,
    .uploader = openvr_vulkan_uploader_new (),
  };

  OpenVRContext *context = openvr_context_get_instance ();
  if (!openvr_context_init_overlay (context))
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  if (!openvr_vulkan_uploader_init_vulkan (self.uploader, true))
    {
      g_printerr ("Unable to initialize Vulkan!\n");
      return false;
    }

  self.pointer = _init_pointer_overlay ();

  if (!_init_intersection_overlay (&self))
    return -1;

  if (!_init_cat_overlay (&self))
    return -1;

  g_timeout_add (20, _overlay_event_cb, &self);

  g_unix_signal_add (SIGINT, _sigint_cb, &self);

  /* start glib main loop */
  g_main_loop_run (self.loop);
  g_main_loop_unref (self.loop);

  _cleanup (&self);

  return 0;
}
