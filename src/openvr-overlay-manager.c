/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gdk/gdk.h>
#include <math.h>

#include "openvr-overlay-manager.h"
#include "openvr-overlay.h"
#include "openvr-math.h"

G_DEFINE_TYPE (OpenVROverlayManager, openvr_overlay_manager, G_TYPE_OBJECT)

enum {
  NO_HOVER_EVENT,
  LAST_SIGNAL
};
static guint overlay_manager_signals[LAST_SIGNAL] = { 0 };

static void
openvr_overlay_manager_finalize (GObject *gobject);

static void
openvr_overlay_manager_class_init (OpenVROverlayManagerClass *klass)
{
  overlay_manager_signals[NO_HOVER_EVENT] =
    g_signal_new ("no-hover-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_FIRST,
                   0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_overlay_manager_finalize;
}

static void
openvr_overlay_manager_init (OpenVROverlayManager *self)
{
  self->reset_transforms = g_hash_table_new (g_direct_hash, g_direct_equal);
  self->hover_state.distance = 1.0f;
  self->hover_state.overlay = NULL;
  self->grab_state.overlay = NULL;
}

OpenVROverlayManager *
openvr_overlay_manager_new (void)
{
  return (OpenVROverlayManager*) g_object_new (OPENVR_TYPE_OVERLAY_MANAGER, 0);
}

void
_free_matrix_cb (gpointer m)
{
  graphene_matrix_free ((graphene_matrix_t*) m);
}

static void
openvr_overlay_manager_finalize (GObject *gobject)
{
  OpenVROverlayManager *self = OPENVR_OVERLAY_MANAGER (gobject);

  GList *matrices = g_hash_table_get_values (self->reset_transforms);
  g_list_free_full (matrices, _free_matrix_cb);

  g_hash_table_unref (self->reset_transforms);

  g_slist_free_full (self->destroy_overlays, g_object_unref);
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

void
openvr_overlay_manager_arrange_reset (OpenVROverlayManager *self)
{
  GSList *l;
  for (l = self->grab_overlays; l != NULL; l = l->next)
    {
      OpenVROverlay *overlay = (OpenVROverlay*) l->data;

      TransformTransition *transition = g_malloc (sizeof *transition);

      graphene_matrix_t *transform =
        g_hash_table_lookup (self->reset_transforms, overlay);

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
openvr_overlay_manager_arrange_sphere (OpenVROverlayManager *self,
                                       uint32_t              grid_width,
                                       uint32_t              grid_height)
{
  float theta_start = M_PI / 2.0f;
  float theta_end = M_PI - M_PI / 8.0f;
  float theta_range = theta_end - theta_start;
  float theta_step = theta_range / grid_width;

  float phi_start = 0;
  float phi_end = M_PI;
  float phi_range = phi_end - phi_start;
  float phi_step = phi_range / grid_height;

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
            (OpenVROverlay*) g_slist_nth_data (self->grab_overlays, i);

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

void
openvr_overlay_manager_add_overlay (OpenVROverlayManager *self,
                                    OpenVROverlay        *overlay,
                                    OpenVROverlayFlags    flags)
{
  /* Freed with manager */
  if (flags & OPENVR_OVERLAY_DESTROY_WITH_PARENT)
    self->destroy_overlays = g_slist_append (self->destroy_overlays, overlay);

  /* Movable overlays */
  if (flags & OPENVR_OVERLAY_GRAB)
    self->grab_overlays = g_slist_append (self->grab_overlays, overlay);

  /* All overlays that can be hovered, includes button overlays */
  if (flags & OPENVR_OVERLAY_HOVER)
    self->hover_overlays = g_slist_append (self->hover_overlays, overlay);

  /* Register reset position */
  graphene_matrix_t *transform = graphene_matrix_alloc ();
  openvr_overlay_get_transform_absolute (overlay, transform);
  g_hash_table_insert (self->reset_transforms, overlay, transform);
}

void
_test_hover (OpenVROverlayManager *self,
             graphene_matrix_t    *pose)
{
  OpenVRHoverEvent *event = g_malloc (sizeof (OpenVRHoverEvent));
  event->distance = FLT_MAX;

  OpenVROverlay *closest = NULL;

  for (GSList *l = self->hover_overlays; l != NULL; l = l->next)
    {
      OpenVROverlay *overlay = (OpenVROverlay*) l->data;

      graphene_point3d_t intersection_point;
      if (openvr_overlay_intersects (overlay, &intersection_point, pose))
        {
          float distance =
            openvr_math_point_matrix_distance (&intersection_point, pose);
          if (distance < event->distance)
            {
              closest = overlay;
              event->distance = distance;
              graphene_matrix_init_from_matrix (&event->pose, pose);
              graphene_point3d_init_from_point (&event->point,
                                                &intersection_point);
            }
        }
    }

    if (closest != NULL)
      {
        /* We now hover over an overlay */
        if (closest != self->hover_state.overlay
            && self->hover_state.overlay != NULL)
          openvr_overlay_emit_hover_end (self->hover_state.overlay);

        self->hover_state.distance = event->distance;
        self->hover_state.overlay = closest;
        graphene_matrix_init_from_matrix (&self->hover_state.pose, pose);

        openvr_overlay_emit_hover (closest, event);
      }
    else
      {
        /* No intersection was found, nothing is hovered */
        g_free (event);

        /* Emit no hover event only if we had hovered something earlier */
        if (self->hover_state.overlay != NULL)
          {
            openvr_overlay_emit_hover_end (self->hover_state.overlay);
            self->hover_state.overlay = NULL;
            g_signal_emit (self, overlay_manager_signals[NO_HOVER_EVENT], 0);
          }
      }
}

void
_drag_overlay (OpenVROverlayManager *self,
               graphene_matrix_t    *pose)
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

  openvr_overlay_set_transform_absolute (self->grab_state.overlay,
                                        &transformed);
}

void
openvr_overlay_manager_drag_start (OpenVROverlayManager *self)
{
  /* Copy hover to grab state */
  self->grab_state.overlay = self->hover_state.overlay;
  graphene_matrix_init_from_matrix (&self->grab_state.pose,
                                    &self->hover_state.pose);
}

void
openvr_overlay_manager_check_grab (OpenVROverlayManager *self)
{
  if (self->hover_state.overlay != NULL)
    openvr_overlay_emit_grab (self->hover_state.overlay);
}

void
openvr_overlay_manager_check_release (OpenVROverlayManager *self)
{
  if (self->grab_state.overlay != NULL)
    openvr_overlay_emit_release (self->grab_state.overlay);
  self->grab_state.overlay = NULL;
}

void
opevr_overlay_manager_update_pose (OpenVROverlayManager *self,
                                   graphene_matrix_t    *pose)
{
  /* Drag test */
  if (self->grab_state.overlay != NULL)
    _drag_overlay (self, pose);
  else
    _test_hover (self, pose);
}
