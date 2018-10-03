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
#include "openvr-io.h"
#include "openvr-compositor.h"
#include "openvr-math.h"
#include "openvr-overlay.h"
#include "openvr-vulkan-uploader.h"
#include "openvr-action.h"
#include "openvr-action-set.h"
#include "openvr-pointer.h"
#include "openvr-intersection.h"

typedef struct Example
{
  OpenVRVulkanUploader *uploader;
  OpenVRVulkanTexture *texture;

  OpenVRActionSet *wm_action_set;

  OpenVRPointer *pointer_overlay;
  OpenVRIntersection *intersection_overlay;
  OpenVROverlay *paint_overlay;

  GdkPixbuf *draw_pixbuf;

  GMainLoop *loop;
} Example;

gboolean
_sigint_cb (gpointer _self)
{
  Example *self = (Example*) _self;
  g_main_loop_quit (self->loop);
  return TRUE;
}

gboolean
_poll_events_cb (gpointer _self)
{
  Example *self = (Example*) _self;

  if (!openvr_action_set_poll (self->wm_action_set))
    return FALSE;

  return TRUE;
}

GdkPixbuf *
_create_draw_pixbuf (uint32_t width, uint32_t height)
{
  guchar * pixels = (guchar*) malloc (sizeof (guchar) * height * width * 4);
  memset (pixels, 255, height * width * 4 * sizeof (guchar));

  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB,
                                                TRUE, 8, width, height,
                                                4 * width, NULL, NULL);
  return pixbuf;
}

GdkPixbuf *
load_gdk_pixbuf (const gchar* name)
{
  GError * error = NULL;
  GdkPixbuf *pixbuf_rgb = gdk_pixbuf_new_from_resource (name, &error);

  if (error != NULL)
    {
      g_printerr ("Unable to read file: %s\n", error->message);
      g_error_free (error);
      return NULL;
    }

  GdkPixbuf *pixbuf = gdk_pixbuf_add_alpha (pixbuf_rgb, false, 0, 0, 0);
  g_object_unref (pixbuf_rgb);
  return pixbuf;
}

typedef struct ColorRGBA
{
  guchar r;
  guchar g;
  guchar b;
  guchar a;
} ColorRGBA;

void
_place_pixel (guchar    *pixels,
              int        n_channels,
              int        rowstride,
              int        x,
              int        y,
              ColorRGBA *color)
{
  guchar *p = pixels + y * rowstride
                     + x * n_channels;

  p[0] = color->r;
  p[1] = color->g;
  p[2] = color->b;
  p[3] = color->a;
}

gboolean
_draw_at_2d_position (Example          *self,
                      PixelSize        *size_pixels,
                      graphene_point_t *position_2d,
                      ColorRGBA        *color,
                      uint32_t          brush_radius)
{
  static GMutex paint_mutex;

  int n_channels = gdk_pixbuf_get_n_channels (self->draw_pixbuf);
  int rowstride = gdk_pixbuf_get_rowstride (self->draw_pixbuf);
  guchar *pixels = gdk_pixbuf_get_pixels (self->draw_pixbuf);

  g_mutex_lock (&paint_mutex);

  for (float x = position_2d->x - (float) brush_radius;
       x <= position_2d->x + (float) brush_radius;
       x++)
    {
      for (float y = position_2d->y - (float) brush_radius;
           y <= position_2d->y + (float) brush_radius;
           y++)
        {
          /* check bounds */
          if (x >= 0 && x <= (float) size_pixels->width &&
              y >= 0 && y <= (float) size_pixels->height)
            {
              graphene_vec2_t put_position;
              graphene_vec2_init (&put_position, x, y);

              graphene_vec2_t brush_center;
              graphene_point_to_vec2 (position_2d, &brush_center);

              graphene_vec2_t distance;
              graphene_vec2_subtract (&put_position, &brush_center, &distance);

              if (graphene_vec2_length (&distance) < brush_radius)
                _place_pixel (pixels, n_channels, rowstride,
                              (int) x, (int) y, color);
            }
        }
    }

  if (!openvr_vulkan_client_upload_pixbuf (OPENVR_VULKAN_CLIENT (self->uploader),
                                           self->texture,
                                           self->draw_pixbuf))
    return FALSE;

  openvr_vulkan_uploader_submit_frame (self->uploader,
                                       self->paint_overlay, self->texture);

  g_mutex_unlock (&paint_mutex);

  return TRUE;
}

static void
_intersection_cb (OpenVROverlay           *overlay,
                  OpenVRIntersectionEvent *event,
                  gpointer                 _self)
{
  Example *self = (Example*) _self;

  // if we have an intersection point, move the pointer overlay there
  if (event->has_intersection)
    {
      openvr_intersection_update (self->intersection_overlay,
                                 &event->transform,
                                 &event->intersection_point);

      PixelSize size_pixels = {
        .width = (guint) gdk_pixbuf_get_width (self->draw_pixbuf),
        .height = (guint) gdk_pixbuf_get_height (self->draw_pixbuf)
      };

      graphene_point_t position_2d;
      if (!openvr_overlay_get_2d_intersection (overlay,
                                              &event->intersection_point,
                                              &size_pixels,
                                              &position_2d))
        return;

      /* check bounds */
      if (position_2d.x < 0 || position_2d.x > size_pixels.width ||
          position_2d.y < 0 || position_2d.y > size_pixels.height)
        return;

      ColorRGBA color = {
        .r = 0,
        .g = 0,
        .b = 0,
        .a = 255
      };

      _draw_at_2d_position (self, &size_pixels, &position_2d, &color, 5);
    }
  else
    {
      openvr_overlay_hide (OPENVR_OVERLAY (self->intersection_overlay));
    }

  free (event);
}

gboolean
_init_draw_overlay (Example *self)
{
  self->draw_pixbuf = _create_draw_pixbuf (1000, 500);
  if (self->draw_pixbuf == NULL)
    return FALSE;

  self->paint_overlay = openvr_overlay_new ();
  openvr_overlay_create (self->paint_overlay, "vulkan.cat", "Vulkan Cat");

  if (!openvr_overlay_is_valid (self->paint_overlay))
    {
      fprintf (stderr, "Overlay unavailable.\n");
      return -1;
    }

  if (!openvr_overlay_set_width_meters (self->paint_overlay, 3.37f))
    return FALSE;

  graphene_point3d_t position = {
    .x = -1,
    .y = 1,
    .z = -3
  };

  graphene_matrix_t transform;
  graphene_matrix_init_translate (&transform, &position);
  openvr_overlay_set_transform_absolute (self->paint_overlay, &transform);

  if (!openvr_overlay_show (self->paint_overlay))
    return -1;

  OpenVRVulkanClient *client = OPENVR_VULKAN_CLIENT (self->uploader);

  self->texture =
    openvr_vulkan_texture_new_from_pixbuf (client->device, self->draw_pixbuf);

  openvr_vulkan_client_upload_pixbuf (client, self->texture, self->draw_pixbuf);

  openvr_vulkan_uploader_submit_frame (self->uploader,
                                       self->paint_overlay, self->texture);

  /* connect glib callbacks */
  g_signal_connect (self->paint_overlay, "intersection-event",
                    (GCallback)_intersection_cb,
                    self);
  return TRUE;
}

void
_cleanup (Example *self)
{
  g_print ("bye\n");

  g_object_unref (self->intersection_overlay);
  g_object_unref (self->pointer_overlay);
  g_object_unref (self->intersection_overlay);
  g_object_unref (self->texture);

  g_object_unref (self->wm_action_set);

  /* destroy context before uploader because context finalization calls
   * VR_ShutdownInternal() which accesses the vulkan device that is being
   * destroyed by uploader finalization
   */

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  g_object_unref (self->uploader);
}

gboolean
_cache_bindings (GString *actions_path)
{
  GString* cache_path = openvr_io_get_cache_path ("openvr-glib");

  if (!openvr_io_create_directory_if_needed (cache_path->str))
    return FALSE;

  if (!openvr_io_write_resource_to_file ("/res/bindings", cache_path->str,
                                         "actions.json", actions_path))
    return FALSE;

  GString *bindings_path = g_string_new ("");
  if (!openvr_io_write_resource_to_file ("/res/bindings", cache_path->str,
                                         "bindings_vive_controller.json",
                                         bindings_path))
    return FALSE;

  g_string_free (bindings_path, TRUE);
  g_string_free (cache_path, TRUE);

  return TRUE;
}

static void
_dominant_hand_cb (OpenVRAction    *action,
                   OpenVRPoseEvent *event,
                   Example         *self)
{
  (void) action;

  /* Update pointer */
  graphene_matrix_t scale_matrix;
  graphene_matrix_init_scale (&scale_matrix, 1.0f, 1.0f, 5.0f);

  graphene_matrix_t scaled;
  graphene_matrix_multiply (&scale_matrix, &event->pose, &scaled);

  openvr_overlay_set_transform_absolute (OPENVR_OVERLAY (self->pointer_overlay),
                                        &scaled);

  /* update intersection */
  openvr_overlay_poll_3d_intersection (self->paint_overlay, &event->pose);

  g_free (event);
}

int
main ()
{
  OpenVRContext *context = openvr_context_get_instance ();
  if (!openvr_context_init_overlay (context))
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  GString *action_manifest_path = g_string_new ("");
  if (!_cache_bindings (action_manifest_path))
    return FALSE;

  g_print ("Resulting manifest path: %s", action_manifest_path->str);

  if (!openvr_action_load_manifest (action_manifest_path->str))
    return FALSE;

  g_string_free (action_manifest_path, TRUE);

  Example self = {
    .loop = g_main_loop_new (NULL, FALSE),
    .wm_action_set = openvr_action_set_new_from_url ("/actions/wm"),
    .uploader = openvr_vulkan_uploader_new (),
  };

  if (!openvr_vulkan_uploader_init_vulkan (self.uploader, true))
    {
      g_printerr ("Unable to initialize Vulkan!\n");
      return false;
    }

  self.pointer_overlay = openvr_pointer_new ();
  if (self.pointer_overlay == NULL)
    return -1;

  self.intersection_overlay = openvr_intersection_new ("/res/crosshair.png");
  if (self.intersection_overlay == NULL)
    return -1;

  if (!_init_draw_overlay (&self))
    return -1;

  openvr_action_set_register (self.wm_action_set, OPENVR_ACTION_POSE,
                              "/actions/wm/in/hand_primary",
                              (GCallback) _dominant_hand_cb, &self);

  g_timeout_add (20, _poll_events_cb, &self);

  g_unix_signal_add (SIGINT, _sigint_cb, &self);

  /* start glib main loop */
  g_main_loop_run (self.loop);
  g_main_loop_unref (self.loop);

  _cleanup (&self);

  return 0;
}
