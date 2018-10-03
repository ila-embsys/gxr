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
#include <glib/gprintf.h>

#include <openvr-glib.h>
#include <math.h>

#include "openvr-context.h"
#include "openvr-compositor.h"
#include "openvr-math.h"
#include "openvr-overlay.h"
#include "openvr-vulkan-uploader.h"

#include "cairo_content.h"

#include "openvr-io.h"
#include "openvr-action.h"
#include "openvr-action-set.h"
#include "openvr-pointer.h"
#include "openvr-intersection.h"

#define GRID_WIDTH 6
#define GRID_HEIGHT 5

typedef struct Example
{
  OpenVRVulkanTexture *texture;

  GSList *cat_overlays;

  GSList *textures_to_free;

  OpenVRPointer *pointer_overlay;
  OpenVRIntersection *intersection;

  OpenVROverlay *current_hover_overlay;
  OpenVROverlay *current_grab_overlay;
  graphene_matrix_t *current_grab_matrix;
  graphene_matrix_t *current_hover_matrix;

  OpenVROverlay *control_overlay_reset;
  OpenVROverlay *control_overlay_sphere;

  GMainLoop *loop;

  GHashTable *initial_overlay_transforms;

  float distance;

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


void
_update_intersection_position (OpenVROverlay      *overlay,
                               graphene_matrix_t  *pose,
                               graphene_point3d_t *intersection_point)
{
  graphene_matrix_t transform;
  graphene_matrix_init_from_matrix (&transform, pose);
  openvr_math_matrix_set_translation (&transform, intersection_point);
  openvr_overlay_set_transform_absolute (overlay, &transform);
  openvr_overlay_show (overlay);
}

gboolean
_test_overlay_intersection (Example *self, graphene_matrix_t *pose)
{
  gboolean has_intersection = FALSE;
  GSList *l;
  gboolean found_intersection = FALSE;
  OpenVROverlay *nearest_intersected = NULL;
  float nearest_dist = 100000.;

  for (l = self->cat_overlays; l != NULL; l = l->next)
    {
      OpenVROverlay *overlay = (OpenVROverlay*) l->data;

      /* TODO: Use intersection callback for this. */
      graphene_point3d_t intersection_point;
      if (openvr_overlay_intersects (overlay, &intersection_point, pose))
        {
          found_intersection = TRUE;

          /* Distance */
          graphene_vec3_t intersection_vec;
          graphene_vec3_init (&intersection_vec,
                               intersection_point.x,
                               intersection_point.y,
                               intersection_point.z);

          graphene_vec3_t controller_position;
          openvr_math_matrix_get_translation (pose, &controller_position);

          graphene_vec3_t distance_vec;
          graphene_vec3_subtract (&controller_position,
                                  &intersection_vec,
                                  &distance_vec);

          self->distance = graphene_vec3_length (&distance_vec);
          // g_print ("distance %.2f\n", distance);

          if (self->distance < nearest_dist)
            {
              nearest_intersected = overlay;
              nearest_dist = self->distance;
            }

          _update_intersection_position (OPENVR_OVERLAY (self->intersection),
                                         pose,
                                        &intersection_point);
        }
    }

  // if we had highlighted an overlay previously, unhighlight it
  if (self->current_hover_overlay != NULL)
    {
      graphene_vec3_t unmarked_color;
      graphene_vec3_init (&unmarked_color, 1.f, 1.f, 1.f);
      openvr_overlay_set_color (self->current_hover_overlay, &unmarked_color);
    }

  // nearest intersected overlay is null when we don't hover over an overlay
  self->current_hover_overlay = nearest_intersected;

  // if we now hover over an overlay, highlight it
  if (self->current_hover_overlay != NULL)
    {
      has_intersection = TRUE;
      graphene_matrix_init_from_matrix (self->current_hover_matrix,
                                        pose);

      graphene_vec3_t marked_color;
      graphene_vec3_init (&marked_color, .8f, .2f, .2f);
      openvr_overlay_set_color (self->current_hover_overlay, &marked_color);
      openvr_pointer_move (self->pointer_overlay, pose, nearest_dist);
    }

  /* Test control overlays */
  if (!found_intersection)
    {
      graphene_point3d_t intersection_point;
      if (openvr_overlay_intersects (self->control_overlay_reset,
                                    &intersection_point,
                                     pose))
        {
          found_intersection = TRUE;

          if (self->current_hover_overlay != NULL)
            {
              graphene_vec3_t unmarked_color;
              graphene_vec3_init (&unmarked_color, 1.f, 1.f, 1.f);
              openvr_overlay_set_color (self->current_hover_overlay,
                                       &unmarked_color);
            }

          self->current_hover_overlay = self->control_overlay_reset;

          graphene_vec3_t marked_color;
          graphene_vec3_init (&marked_color, .2f, .2f, .8f);
          openvr_overlay_set_color (self->control_overlay_reset, &marked_color);
        }
    }

  if (!found_intersection)
    {
      graphene_point3d_t intersection_point;
      if (openvr_overlay_intersects (self->control_overlay_sphere,
                                    &intersection_point,
                                     pose))
        {
          found_intersection = TRUE;

          if (self->current_hover_overlay != NULL)
            {
              graphene_vec3_t unmarked_color;
              graphene_vec3_init (&unmarked_color, 1.f, 1.f, 1.f);
              openvr_overlay_set_color (self->current_hover_overlay,
                                       &unmarked_color);
            }

          self->current_hover_overlay = self->control_overlay_sphere;

          graphene_vec3_t marked_color;
          graphene_vec3_init (&marked_color, .2f, .2f, .8f);
          openvr_overlay_set_color (self->control_overlay_sphere,
                                   &marked_color);
        }
    }

  if (!found_intersection)
    {
      if (self->current_hover_overlay != NULL)
        {
          graphene_vec3_t unmarked_color;
          graphene_vec3_init (&unmarked_color, 1.f, 1.f, 1.f);
          openvr_overlay_set_color (self->current_hover_overlay,
                                   &unmarked_color);
          self->current_hover_overlay = NULL;
        }

      openvr_overlay_hide (OPENVR_OVERLAY (self->intersection));
    }

  return has_intersection;
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
    _update_intersection_position (OPENVR_OVERLAY (self->intersection),
                                  &event->transform,
                                  &event->intersection_point);
  else
    openvr_overlay_hide (OPENVR_OVERLAY (self->intersection));

  // TODO: because this is a custom event with a struct that has been allocated
  // specifically for us, we need to free it. Maybe reuse?
  free (event);
}

typedef struct TransformTransition {
  OpenVROverlay *overlay;
  graphene_matrix_t from;
  graphene_matrix_t to;
  float interpolate;
} TransformTransition;

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
_init_cat_overlays (Example *self)
{
  GdkPixbuf *pixbuf = load_gdk_pixbuf ("/res/cat.jpg");
  if (pixbuf == NULL)
    return -1;

  OpenVRVulkanClient *client = OPENVR_VULKAN_CLIENT (self->uploader);

  self->texture = openvr_vulkan_texture_new_from_pixbuf (client->device, pixbuf);

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
            fprintf (stderr, "Overlay unavailable.\n");
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

        self->cat_overlays = g_slist_append (self->cat_overlays, cat);

        if (!openvr_overlay_show (cat))
          return -1;

        g_signal_connect (cat, "intersection-event",
                          (GCallback) _intersection_cb, self);
        i++;

      }

  g_object_unref (pixbuf);

  return TRUE;
}

cairo_surface_t*
_create_cairo_surface (unsigned char *image, uint32_t width,
                       uint32_t height, const gchar *text)
{
  cairo_surface_t *surface =
    cairo_image_surface_create_for_data (image,
                                         CAIRO_FORMAT_ARGB32,
                                         width, height,
                                         width * 4);

  cairo_t *cr = cairo_create (surface);

  cairo_rectangle (cr, 0, 0, width, height);
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_fill (cr);

  double r0;
  if (width < height)
    r0 = (double) width / 3.0;
  else
    r0 = (double) height / 3.0;

  double radius = r0 * 3.0;
  double r1 = r0 * 5.0;

  double center_x = (double) width / 2.0;
  double center_y = (double) height / 2.0;

  double cx0 = center_x - r0 / 2.0;
  double cy0 = center_y - r0;
  double cx1 = center_x - r0;
  double cy1 = center_y - r0;

  cairo_pattern_t *pat = cairo_pattern_create_radial (cx0, cy0, r0,
                                                      cx1, cy1, r1);
  cairo_pattern_add_color_stop_rgba (pat, 0, .3, .3, .3, 1);
  cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0, 0, 1);
  cairo_set_source (cr, pat);
  cairo_arc (cr, center_x, center_y, radius, 0, 2 * M_PI);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  cairo_select_font_face (cr, "Sans",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size (cr, 52.0);

  cairo_text_extents_t extents;
  cairo_text_extents (cr, text, &extents);

  cairo_move_to (cr,
                 center_x - extents.width / 2,
                 center_y  - extents.height / 2);
  cairo_set_source_rgb (cr, 0.9, 0.9, 0.9);
  cairo_show_text (cr, text);

  cairo_destroy (cr);

  return surface;
}

gboolean
_init_control_overlay (Example              *self,
                       OpenVROverlay       **overlay,
                       gchar                *id,
                       const gchar          *text,
                       graphene_matrix_t    *transform)
{
  guint width = 200;
  guint height = 200;
  unsigned char image[4 * width * height];
  cairo_surface_t* surface = _create_cairo_surface (image, width,
                                                    height, text);

  if (surface == NULL) {
    fprintf (stderr, "Could not create cairo surface.\n");
    return -1;
  }

  if (!openvr_vulkan_uploader_init_vulkan (self->uploader, true))
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return FALSE;
  }

  OpenVRVulkanClient *client = OPENVR_VULKAN_CLIENT (self->uploader);

  OpenVRVulkanTexture *cairo_texture =
    openvr_vulkan_texture_new_from_cairo_surface (client->device, surface);

  openvr_vulkan_client_upload_cairo_surface (client, cairo_texture, surface);

  /* create openvr overlay */
  *overlay = openvr_overlay_new ();
  openvr_overlay_create_width (*overlay, id, id, 0.5f);

  if (!openvr_overlay_is_valid (*overlay))
  {
    g_printerr ("Overlay unavailable.\n");
    return FALSE;
  }

  openvr_vulkan_uploader_submit_frame (self->uploader,
                                       *overlay, cairo_texture);

  self->textures_to_free = g_slist_append (self->textures_to_free,
                                           cairo_texture);

  openvr_overlay_set_transform_absolute (*overlay, transform);

  if (!openvr_overlay_show (*overlay))
    return -1;

  return TRUE;
}

gboolean
_init_control_overlays (Example *self)
{
  graphene_point3d_t position = {
    .x =  0.0f,
    .y =  0.0f,
    .z = -1.0f
  };

  graphene_matrix_t transform;
  graphene_matrix_init_translate (&transform, &position);

  if (!_init_control_overlay (self, &self->control_overlay_reset,
                              "control.reset", "Reset", &transform))
    return FALSE;

  graphene_point3d_t position_sphere = {
    .x =  0.5f,
    .y =  0.0f,
    .z = -1.0f
  };

  graphene_matrix_t transform_sphere;
  graphene_matrix_init_translate (&transform_sphere, &position_sphere);

  if (!_init_control_overlay (self, &self->control_overlay_sphere,
                              "control.sphere", "Sphere", &transform_sphere))
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

static void
_dominant_hand_cb (OpenVRAction    *action,
                   OpenVRPoseEvent *event,
                   gpointer         _self)
{
  (void) action;

  Example *self = (Example*) _self;

  /* Update pointer */
  graphene_matrix_t scale_matrix;
  graphene_matrix_init_scale (&scale_matrix, 1.0f, 1.0f, 5.0f);

  graphene_matrix_t scaled;
  graphene_matrix_multiply (&scale_matrix, &event->pose, &scaled);

  /* Drag */
  if (self->current_grab_overlay != NULL)
    {
      graphene_point3d_t translation_point;
      graphene_point3d_init (&translation_point, .0f, .0f, -self->distance);

      graphene_matrix_t transformation_matrix;
      graphene_matrix_init_translate (&transformation_matrix,
                                      &translation_point);

      graphene_matrix_t transformed;
      graphene_matrix_multiply (&transformation_matrix,
                                &event->pose,
                                &transformed);

      openvr_overlay_set_transform_absolute (self->current_grab_overlay,
                                             &transformed);
      openvr_pointer_move (self->pointer_overlay, &event->pose, self->distance);
    }
  else
    {
      gboolean has_intersection = _test_overlay_intersection (self,
                                                              &event->pose);
      if (!has_intersection)
          openvr_pointer_move (self->pointer_overlay, &event->pose,
                               self->pointer_default_length);
    }

  g_free (event);
}

static void
_grab_cb (OpenVRAction       *action,
          OpenVRDigitalEvent *event,
          gpointer            _self)
{
  (void) action;

  Example *self = (Example*) _self;

  //g_print ("grab: Active %d | State %d | Changed %d\n",
  //         event->active, event->state, event->changed);

  if (event->changed)
    {
      if (event->state == 1)
        {
          // Press
          if (self->current_hover_overlay != NULL)
            {
              if (self->current_hover_overlay == self->control_overlay_reset)
                {
                  _reset_cat_overlays (self);
                }
              else if (self->current_hover_overlay
                       == self->control_overlay_sphere)
                {
                  _position_cat_overlays_sphere (self);
                }
              else
                {
                  self->current_grab_overlay = self->current_hover_overlay;

                  graphene_matrix_init_from_matrix (self->current_grab_matrix,
                                                    self->current_hover_matrix);

                  openvr_overlay_hide (OPENVR_OVERLAY (self->intersection));

                  graphene_vec3_t marked_color;
                  graphene_vec3_init (&marked_color, .2f, .8f, .2f);
                  openvr_overlay_set_color (self->current_grab_overlay,
                                           &marked_color);
                }
            }
        }
      else
        {
          // Releae
          if (self->current_grab_overlay != NULL)
            {
              graphene_vec3_t unmarked_color;
              graphene_vec3_init (&unmarked_color, 1.f, 1.f, 1.f);
              openvr_overlay_set_color (self->current_grab_overlay,
                                       &unmarked_color);
            }
          self->current_grab_overlay = NULL;
          //
        }
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
  g_object_unref (self->uploader);

  g_object_unref (self->wm_action_set);

  g_slist_free_full (self->cat_overlays, g_object_unref);
  g_slist_free_full (self->textures_to_free, g_object_unref);

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);
}


int
main ()
{
  /* init openvr */
  if (!_init_openvr ())
    return -1;

  GString *action_manifest_path = g_string_new ("");
  if (!_cache_bindings (action_manifest_path))
    return FALSE;

  g_print ("Resulting manifest path: %s", action_manifest_path->str);

  if (!openvr_action_load_manifest (action_manifest_path->str))
    return FALSE;

  g_string_free (action_manifest_path, TRUE);

  Example self = {
    .loop = g_main_loop_new (NULL, FALSE),
    .current_grab_matrix = graphene_matrix_alloc (),
    .current_hover_matrix = graphene_matrix_alloc (),
    .wm_action_set = openvr_action_set_new_from_url ("/actions/wm"),
    .distance = 1.f,
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

  if (!_init_control_overlays (&self))
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
