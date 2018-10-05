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
#include <glib/gprintf.h>

#include <openvr-glib.h>

#include "openvr-context.h"
#include "openvr-compositor.h"
#include "openvr-math.h"
#include "openvr-overlay.h"
#include "openvr-vulkan-uploader.h"
#include "openvr-io.h"
#include "openvr-action.h"
#include "openvr-action-set.h"
#include "openvr-pointer.h"
#include "openvr-intersection.h"
#include "openvr-button.h"
#include "openvr-overlay-manager.h"

#define GRID_WIDTH 6
#define GRID_HEIGHT 5

typedef struct Example
{
  OpenVRVulkanTexture *texture;

  OpenVROverlayManager *manager;

  OpenVRPointer *pointer_overlay;
  OpenVRIntersection *intersection;

  OpenVRButton *button_reset;
  OpenVRButton *button_sphere;

  GMainLoop *loop;

  float pointer_default_length;

  OpenVRActionSet *wm_action_set;

  OpenVRVulkanUploader *uploader;
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

void
_overlay_unmark (OpenVROverlay *overlay)
{
  graphene_vec3_t unmarked_color;
  graphene_vec3_init (&unmarked_color, 1.f, 1.f, 1.f);
  openvr_overlay_set_color (overlay, &unmarked_color);
}

void
_overlay_mark_blue (OpenVROverlay *overlay)
{
  graphene_vec3_t marked_color;
  graphene_vec3_init (&marked_color, .2f, .2f, .8f);
  openvr_overlay_set_color (overlay, &marked_color);
}

void
_overlay_mark_green (OpenVROverlay *overlay)
{
  graphene_vec3_t marked_color;
  graphene_vec3_init (&marked_color, .2f, .8f, .2f);
  openvr_overlay_set_color (overlay, &marked_color);
}

gboolean
_init_cat_overlays (Example *self)
{
  GdkPixbuf *pixbuf = load_gdk_pixbuf ("/res/cat.jpg");
  if (pixbuf == NULL)
    return -1;

  OpenVRVulkanClient *client = OPENVR_VULKAN_CLIENT (self->uploader);

  self->texture =
    openvr_vulkan_texture_new_from_pixbuf (client->device, pixbuf);

  openvr_vulkan_client_upload_pixbuf (client, self->texture, pixbuf);

  float width = .5f;

  float pixbuf_aspect = (float) gdk_pixbuf_get_width (pixbuf) /
                        (float) gdk_pixbuf_get_height (pixbuf);

  float height = width / pixbuf_aspect;

  g_print ("pixbuf: %.2f x %.2f\n", (float) gdk_pixbuf_get_width (pixbuf),
                                    (float) gdk_pixbuf_get_height (pixbuf));
  g_print ("meters: %.2f x %.2f\n", width, height);


  uint32_t i = 0;

  for (float x = 0; x < GRID_WIDTH * width; x += width)
    for (float y = 0; y < GRID_HEIGHT * height; y += height)
      {
        OpenVROverlay *cat = openvr_overlay_new ();

        char overlay_name[10];
        g_sprintf (overlay_name, "cat%d", i);

        openvr_overlay_create_width (cat, overlay_name, "A Cat", width);

        if (!openvr_overlay_is_valid (cat))
          {
            g_printerr ("Overlay unavailable.\n");
            return -1;
          }

        graphene_point3d_t position = {
          .x = x,
          .y = y,
          .z = -3
        };
        openvr_overlay_set_translation (cat, &position);
        openvr_overlay_manager_add_overlay (self->manager, cat,
                                            OPENVR_OVERLAY_HOVER |
                                            OPENVR_OVERLAY_GRAB |
                                            OPENVR_OVERLAY_DESTROY_WITH_PARENT);

        openvr_vulkan_uploader_submit_frame (self->uploader, cat,
                                             self->texture);

        if (!openvr_overlay_show (cat))
          return -1;

        i++;
      }

  g_object_unref (pixbuf);

  return TRUE;
}

gboolean
_init_buttons (Example *self)
{
  graphene_point3d_t position = {
    .x =  0.0f,
    .y =  0.0f,
    .z = -1.0f
  };

  graphene_matrix_t transform;
  graphene_matrix_init_translate (&transform, &position);

  self->button_reset = openvr_button_new ("control.reset", "Reset");
  if (self->button_reset == NULL)
    return FALSE;

  openvr_overlay_set_transform_absolute (OPENVR_OVERLAY (self->button_reset),
                                        &transform);

  openvr_overlay_manager_add_overlay (self->manager,
                                      OPENVR_OVERLAY (self->button_reset),
                                      OPENVR_OVERLAY_HOVER);

  if (!openvr_overlay_set_width_meters (
        OPENVR_OVERLAY (self->button_reset), 0.5f))
    return FALSE;

  graphene_point3d_t position_sphere = {
    .x =  0.5f,
    .y =  0.0f,
    .z = -1.0f
  };

  graphene_matrix_t transform_sphere;
  graphene_matrix_init_translate (&transform_sphere, &position_sphere);

  self->button_sphere = openvr_button_new ("control.sphere", "Sphere");
  if (self->button_sphere == NULL)
    return FALSE;

  openvr_overlay_set_transform_absolute (OPENVR_OVERLAY (self->button_sphere),
                                        &transform_sphere);

  openvr_overlay_manager_add_overlay (self->manager,
                                      OPENVR_OVERLAY (self->button_sphere),
                                      OPENVR_OVERLAY_HOVER);

  if (!openvr_overlay_set_width_meters (
        OPENVR_OVERLAY (self->button_sphere), 0.5f))
    return FALSE;

  return TRUE;
}

void
_hover_cb (OpenVROverlayManager *manager,
           OpenVRHoverEvent     *event,
           gpointer             _self)
{
  (void) manager;

  Example *self = (Example*) _self;

  /* If we had highlighted an overlay previously, unhighlight it */
  if (event->previous_overlay != NULL)
    _overlay_unmark (event->previous_overlay);

  /* highlight it */
  _overlay_mark_blue (event->overlay);

  /* update pointer and intersection overlays */
  openvr_intersection_update (self->intersection, &event->pose, &event->point);
  openvr_pointer_move (self->pointer_overlay, &event->pose, event->distance);
}

void
_no_hover_cb (OpenVROverlayManager *manager,
              OpenVRNoHoverEvent   *event,
              gpointer             _self)
{
  (void) manager;

  /* If we had highlighted an overlay previously, unhighlight it */
  if (event->previous_overlay != NULL)
    _overlay_unmark (event->previous_overlay);

  Example *self = (Example*) _self;
  openvr_overlay_hide (OPENVR_OVERLAY (self->intersection));
  openvr_pointer_move (self->pointer_overlay, &event->pose,
                       self->pointer_default_length);
}

static void
_dominant_hand_cb (OpenVRAction    *action,
                   OpenVRPoseEvent *event,
                   gpointer         _self)
{
  (void) action;
  Example *self = (Example*) _self;

  /* Drag test */
  if (openvr_overlay_manager_is_grabbing (self->manager))
    {
      openvr_overlay_manager_drag_overlay (self->manager, &event->pose);
      float distance =
        openvr_overlay_manager_get_hover_distance (self->manager);
      openvr_pointer_move (self->pointer_overlay, &event->pose, distance);
    }
  else
    openvr_overlay_manager_test_hover (self->manager, &event->pose);

  g_free (event);
}

void
_grab_press (Example *self)
{
  if (!openvr_overlay_manager_is_hovering (self->manager))
    return;

  /* Reset button pressed */
  if (openvr_overlay_manager_is_hovered (self->manager,
                                         OPENVR_OVERLAY (self->button_reset)))
    {
      openvr_overlay_manager_arrange_reset (self->manager);
    }
  /* Sphere button pressed */
  else if
  (openvr_overlay_manager_is_hovered (self->manager,
                                      OPENVR_OVERLAY (self->button_sphere)))
    {
      openvr_overlay_manager_arrange_sphere (self->manager,
                                             GRID_WIDTH, GRID_HEIGHT);
    }
  /* Overlay grabbed */
  else
    {
      openvr_overlay_manager_drag_start (self->manager);
      openvr_overlay_hide (OPENVR_OVERLAY (self->intersection));
      _overlay_mark_green (self->manager->grab_state.overlay);
    }
}

void
_grab_release (Example *self)
{
  OpenVROverlay *last_grabbed = openvr_overlay_manager_grab_end (self->manager);
  if (last_grabbed != NULL)
    _overlay_unmark (last_grabbed);
}

static void
_grab_cb (OpenVRAction       *action,
          OpenVRDigitalEvent *event,
          gpointer            _self)
{
  (void) action;

  Example *self = (Example*) _self;

  if (event->changed)
    {
      if (event->state == 1)
        _grab_press (self);
      else
        _grab_release (self);
    }

  g_free (event);
}

void
_cleanup (Example *self)
{
  g_main_loop_unref (self->loop);

  g_print ("bye\n");

  g_object_unref (self->pointer_overlay);
  g_object_unref (self->intersection);
  g_object_unref (self->texture);

  g_object_unref (self->button_reset);
  g_object_unref (self->button_sphere);

  g_object_unref (self->wm_action_set);

  g_object_unref (self->manager);

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  g_object_unref (self->uploader);
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

  if (!openvr_io_load_cached_action_manifest ("openvr-glib",
                                              "/res/bindings",
                                              "actions.json",
                                              "bindings_vive_controller.json",
                                              NULL))
    return -1;

  Example self = {
    .loop = g_main_loop_new (NULL, FALSE),
    .wm_action_set = openvr_action_set_new_from_url ("/actions/wm"),
    .manager = openvr_overlay_manager_new (),
    .pointer_default_length = 5.0
  };

  self.uploader = openvr_vulkan_uploader_new ();
  if (!openvr_vulkan_uploader_init_vulkan (self.uploader, true))
    {
      g_printerr ("Unable to initialize Vulkan!\n");
      return false;
    }

  self.pointer_overlay = openvr_pointer_new ();
  if (self.pointer_overlay == NULL)
    return -1;

  self.intersection = openvr_intersection_new ("/res/crosshair.png");
  if (self.intersection == NULL)
    return -1;

  if (!_init_cat_overlays (&self))
    return -1;

  if (!_init_buttons (&self))
    return -1;

  openvr_action_set_register (self.wm_action_set, OPENVR_ACTION_POSE,
                              "/actions/wm/in/hand_primary",
                              (GCallback) _dominant_hand_cb, &self);

  openvr_action_set_register (self.wm_action_set, OPENVR_ACTION_DIGITAL,
                              "/actions/wm/in/grab_window",
                              (GCallback) _grab_cb, &self);

  g_signal_connect (self.manager, "hover-event",
                    (GCallback) _hover_cb, &self);
  g_signal_connect (self.manager, "no-hover-event",
                    (GCallback) _no_hover_cb, &self);

  g_timeout_add (20, _poll_events_cb, &self);

  g_unix_signal_add (SIGINT, _sigint_cb, &self);

  /* start glib main loop */
  g_main_loop_run (self.loop);

  _cleanup (&self);

  return 0;
}
