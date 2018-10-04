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
#include <math.h>

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

#define GRID_WIDTH 6
#define GRID_HEIGHT 5

typedef struct Intersection
{
  OpenVROverlay     *overlay;
  graphene_point3d_t point;
  float              distance;
} Intersection;

typedef struct TransformTransition {
  OpenVROverlay *overlay;
  graphene_matrix_t from;
  graphene_matrix_t to;
  float interpolate;
} TransformTransition;

typedef struct HoverState {
  OpenVROverlay    *overlay;
  graphene_matrix_t pose;
  float             distance;
} HoverState;

typedef struct Example
{
  OpenVRVulkanTexture *texture;

  GSList *cat_overlays;
  GSList *hover_overlays;

  OpenVRPointer *pointer_overlay;
  OpenVRIntersection *intersection;

  HoverState hover_state;

  OpenVROverlay *current_grab_overlay;
  graphene_matrix_t *current_grab_matrix;

  OpenVRButton *button_reset;
  OpenVRButton *button_sphere;

  GMainLoop *loop;

  GHashTable *initial_overlay_transforms;

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

float
_point_matrix_distance (graphene_point3d_t *intersection_point,
                        graphene_matrix_t  *pose)
{
  graphene_vec3_t intersection_vec;
  graphene_point3d_to_vec3 (intersection_point, &intersection_vec);

  graphene_vec3_t pose_translation;
  openvr_math_matrix_get_translation (pose, &pose_translation);

  graphene_vec3_t distance_vec;
  graphene_vec3_subtract (&pose_translation,
                          &intersection_vec,
                          &distance_vec);

  return graphene_vec3_length (&distance_vec);
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

static void
_intersection_cb (OpenVROverlay           *overlay,
                  OpenVRIntersectionEvent *event,
                  gpointer                 _self)
{
  (void) overlay;

  Example *self = (Example*) _self;

  /* If we have an intersection point, move the pointer overlay there */
  if (event->has_intersection)
    openvr_intersection_update (self->intersection,
                               &event->transform,
                               &event->intersection_point);
  else
    openvr_overlay_hide (OPENVR_OVERLAY (self->intersection));

  free (event);
}

gboolean
_interpolate_cb (gpointer _transition)
{
  TransformTransition *transition = (TransformTransition *) _transition;

  graphene_matrix_t interpolated;
  openvr_math_matrix_interpolate (&transition->from,
                                  &transition->to,
                                   transition->interpolate,
                                  &interpolated);

  openvr_overlay_set_transform_absolute (transition->overlay, &interpolated);

  transition->interpolate += 0.03f;

  if (transition->interpolate > 1)
    {
      openvr_overlay_set_transform_absolute (transition->overlay,
                                             &transition->to);
      g_free (transition);
      return FALSE;
    }

  return TRUE;
}

static void
_reset_cat_overlays (Example *self)
{
  GSList *l;
  for (l = self->cat_overlays; l != NULL; l = l->next)
    {
      OpenVROverlay *overlay = (OpenVROverlay*) l->data;

      TransformTransition *transition = g_malloc (sizeof *transition);

      graphene_matrix_t *transform =
        g_hash_table_lookup (self->initial_overlay_transforms, overlay);

      openvr_overlay_get_transform_absolute (overlay, &transition->from);

      if (!openvr_math_matrix_equals (&transition->from, transform))
        {
          transition->interpolate = 0;
          transition->overlay = overlay;

          graphene_matrix_init_from_matrix (&transition->to, transform);

          g_timeout_add (20, _interpolate_cb, transition);
        }
      else
        {
          g_free (transition);
        }
    }
}

gboolean
_position_cat_overlays_sphere (Example *self)
{
  float theta_start = M_PI / 2.0f;
  float theta_end = M_PI - M_PI / 8.0f;
  float theta_range = theta_end - theta_start;
  float theta_step = theta_range / GRID_HEIGHT;

  float phi_start = 0;
  float phi_end = M_PI;
  float phi_range = phi_end - phi_start;
  float phi_step = phi_range / GRID_WIDTH;

  guint i = 0;

  for (float theta = theta_start; theta < theta_end; theta += theta_step)
    {
      /* TODO: don't need hack 0.01 to check phi range */
      for (float phi = phi_start; phi < phi_end - 0.01; phi += phi_step)
        {
          TransformTransition *transition = g_malloc (sizeof *transition);

          float radius = 3.0f;

          float const x = sin (theta) * cos (phi);
          float const y = cos (theta);
          float const z = sin (phi) * sin (theta);

          graphene_matrix_t transform;

          graphene_vec3_t position;
          graphene_vec3_init (&position,
                              x * radius,
                              y * radius,
                              z * radius);

          graphene_matrix_init_look_at (&transform,
                                        &position,
                                        graphene_vec3_zero (),
                                        graphene_vec3_y_axis ());

          OpenVROverlay *overlay =
            (OpenVROverlay*) g_slist_nth_data (self->cat_overlays, i);

          if (overlay == NULL)
            {
              g_printerr ("Overlay %d does not exist!\n", i);
              return FALSE;
            }

          i++;

          openvr_overlay_get_transform_absolute (overlay, &transition->from);

          if (!openvr_math_matrix_equals (&transition->from, &transform))
            {
              transition->interpolate = 0;
              transition->overlay = overlay;

              graphene_matrix_init_from_matrix (&transition->to, &transform);

              g_timeout_add (20, _interpolate_cb, transition);
            }
          else
            {
              g_free (transition);
            }
        }
    }

  return TRUE;
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

  self->initial_overlay_transforms =
    g_hash_table_new (g_direct_hash, g_direct_equal);

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

        graphene_matrix_t *transform = graphene_matrix_alloc ();
        graphene_matrix_init_translate (transform, &position);

        g_hash_table_insert (self->initial_overlay_transforms, cat, transform);

        openvr_overlay_set_transform_absolute (cat, transform);

        openvr_overlay_set_mouse_scale (cat,
                                        (float) gdk_pixbuf_get_width (pixbuf),
                                        (float) gdk_pixbuf_get_height (pixbuf));

        openvr_vulkan_uploader_submit_frame (self->uploader, cat,
                                             self->texture);

        /* Managed and draggable overlays */
        self->cat_overlays = g_slist_append (self->cat_overlays, cat);

        /* All overlays that can be hovered, includes button overlays */
        self->hover_overlays = g_slist_append (self->hover_overlays, cat);

        if (!openvr_overlay_show (cat))
          return -1;

        g_signal_connect (cat, "intersection-event",
                          (GCallback) _intersection_cb, self);
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

  self->hover_overlays = g_slist_append (self->hover_overlays,
                                         OPENVR_OVERLAY (self->button_reset));

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

  self->hover_overlays = g_slist_append (self->hover_overlays,
                                         OPENVR_OVERLAY (self->button_sphere));

  if (!openvr_overlay_set_width_meters (
        OPENVR_OVERLAY (self->button_sphere), 0.5f))
    return FALSE;

  return TRUE;
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

gboolean
_load_actions_manifest ()
{
  GString *action_manifest_path = g_string_new ("");
  if (!_cache_bindings (action_manifest_path))
    return FALSE;

  g_print ("Resulting manifest path: %s", action_manifest_path->str);

  if (!openvr_action_load_manifest (action_manifest_path->str))
    return FALSE;

  g_string_free (action_manifest_path, TRUE);

  return TRUE;
}

gboolean
_test_overlay_intersection (Example *self, graphene_matrix_t *pose)
{
  /* If we had highlighted an overlay previously, unhighlight it */
  if (self->hover_state.overlay != NULL)
    _overlay_unmark (self->hover_state.overlay);

  /* Test cat overlay intersection */
  Intersection nearest_intersection = {
    .distance = FLT_MAX,
    .overlay = NULL,
  };

  for (GSList *l = self->hover_overlays; l != NULL; l = l->next)
    {
      OpenVROverlay *overlay = (OpenVROverlay*) l->data;

      /* TODO: Use intersection callback for this. */
      graphene_point3d_t intersection_point;
      if (openvr_overlay_intersects (overlay, &intersection_point, pose))
        {
          float distance = _point_matrix_distance (&intersection_point, pose);
          if (distance < nearest_intersection.distance)
            {
              nearest_intersection.overlay = overlay;
              nearest_intersection.distance = distance;
              graphene_point3d_init_from_point (&nearest_intersection.point,
                                                &intersection_point);
            }
        }
    }

  if (nearest_intersection.overlay != NULL)
    {
      /* We now hover over an overlay */
      self->hover_state.distance = nearest_intersection.distance;
      self->hover_state.overlay = nearest_intersection.overlay;
      graphene_matrix_init_from_matrix (&self->hover_state.pose, pose);

      /* highlight it */
      _overlay_mark_blue (self->hover_state.overlay);

      /* update pointer and intersection overlays */
      openvr_intersection_update (self->intersection, pose,
                                 &nearest_intersection.point);
      openvr_pointer_move (self->pointer_overlay, pose,
                           nearest_intersection.distance);
      return TRUE;
    }

  /* No intersection was found, nothing is hovered */
  self->hover_state.overlay = NULL;
  openvr_overlay_hide (OPENVR_OVERLAY (self->intersection));
  openvr_pointer_move (self->pointer_overlay, pose,
                       self->pointer_default_length);

  return FALSE;
}

void
_drag_overlay (Example *self, graphene_matrix_t *pose)
{
  graphene_point3d_t translation_point;
  graphene_point3d_init (&translation_point,
                         .0f, .0f, -self->hover_state.distance);

  graphene_matrix_t transformation_matrix;
  graphene_matrix_init_translate (&transformation_matrix, &translation_point);

  graphene_matrix_t transformed;
  graphene_matrix_multiply (&transformation_matrix,
                             pose,
                            &transformed);

  openvr_overlay_set_transform_absolute (self->current_grab_overlay,
                                        &transformed);
  openvr_pointer_move (self->pointer_overlay, pose, self->hover_state.distance);
}

static void
_dominant_hand_cb (OpenVRAction    *action,
                   OpenVRPoseEvent *event,
                   gpointer         _self)
{
  (void) action;
  Example *self = (Example*) _self;

  /* Drag test */
  if (self->current_grab_overlay != NULL)
    _drag_overlay (self, &event->pose);
  else
    _test_overlay_intersection (self, &event->pose);

  g_free (event);
}

void
_grab_press (Example *self)
{
  if (self->hover_state.overlay == NULL)
    return;

  /* Reset button pressed */
  if (self->hover_state.overlay == OPENVR_OVERLAY (self->button_reset))
    {
      _reset_cat_overlays (self);
    }
  /* Sphere button pressed */
  else if (self->hover_state.overlay  == OPENVR_OVERLAY (self->button_sphere))
    {
      _position_cat_overlays_sphere (self);
    }
  /* Overlay grabbed */
  else
    {
      self->current_grab_overlay = self->hover_state.overlay;

      graphene_matrix_init_from_matrix (self->current_grab_matrix,
                                       &self->hover_state.pose);

      openvr_overlay_hide (OPENVR_OVERLAY (self->intersection));

      _overlay_mark_green (self->current_grab_overlay);
    }
}

void
_grab_release (Example *self)
{
  if (self->current_grab_overlay != NULL)
    _overlay_unmark (self->current_grab_overlay);
  self->current_grab_overlay = NULL;
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

  g_slist_free_full (self->cat_overlays, g_object_unref);

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

  if (!_load_actions_manifest ())
    return -1;

  Example self = {
    .loop = g_main_loop_new (NULL, FALSE),
    .current_grab_matrix = graphene_matrix_alloc (),
    .wm_action_set = openvr_action_set_new_from_url ("/actions/wm"),
    .hover_state = {
      .distance = 1.0f,
      .overlay = NULL
    },
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

  g_timeout_add (20, _poll_events_cb, &self);

  g_unix_signal_add (SIGINT, _sigint_cb, &self);

  /* start glib main loop */
  g_main_loop_run (self.loop);

  _cleanup (&self);

  return 0;
}
