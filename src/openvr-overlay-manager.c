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
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_overlay_manager_finalize;
}

static void
openvr_overlay_manager_init (OpenVROverlayManager *self)
{
  self->reset_transforms = g_hash_table_new (g_direct_hash, g_direct_equal);
  for (int i = 0; i < OPENVR_CONTROLLER_COUNT; i++)
    {
      self->hover_state[i].distance = 1.0f;
      self->hover_state[i].overlay = NULL;
      self->grab_state[i].overlay = NULL;
    }
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
      g_object_unref (transition->overlay);
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
          g_object_ref (overlay);

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
              g_object_ref (overlay);

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
openvr_overlay_manager_save_reset_transform (OpenVROverlayManager *self,
                                             OpenVROverlay        *overlay)
{
  graphene_matrix_t *transform =
    g_hash_table_lookup (self->reset_transforms, overlay);
  openvr_overlay_get_transform_absolute (overlay, transform);
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

  g_object_ref (overlay);
}

void
openvr_overlay_manager_remove_overlay (OpenVROverlayManager *self,
                                       OpenVROverlay        *overlay)
{
  self->destroy_overlays = g_slist_remove (self->destroy_overlays, overlay);
  self->grab_overlays = g_slist_remove (self->grab_overlays, overlay);
  self->hover_overlays = g_slist_remove (self->hover_overlays, overlay);
  g_hash_table_remove (self->reset_transforms, overlay);

  g_object_unref (overlay);
}

static gboolean
openvr_overlay_get_2d_offset (OpenVROverlay      *overlay,
                              graphene_point3d_t *intersection_point,
                              graphene_point_t   *position_2d)
{
  graphene_matrix_t transform;
  openvr_overlay_get_transform_absolute (overlay, &transform);

  graphene_matrix_t inverse_transform;
  graphene_matrix_inverse (&transform, &inverse_transform);

  graphene_point3d_t intersection_origin;
  graphene_matrix_transform_point3d (&inverse_transform,
                                      intersection_point,
                                     &intersection_origin);

  graphene_point_init (position_2d,
                      intersection_origin.x,
                      intersection_origin.y);
  return TRUE;
}

void
_test_hover (OpenVROverlayManager *self,
             graphene_matrix_t    *pose,
             int                   controller_index)
{
  OpenVRHoverEvent *hover_event = g_malloc (sizeof (OpenVRHoverEvent));
  hover_event->distance = FLT_MAX;

  OpenVROverlay *closest = NULL;

  for (GSList *l = self->hover_overlays; l != NULL; l = l->next)
    {
      OpenVROverlay *overlay = (OpenVROverlay*) l->data;

      graphene_point3d_t intersection_point;
      if (openvr_overlay_intersects (overlay, &intersection_point, pose))
        {
          float distance =
            openvr_math_point_matrix_distance (&intersection_point, pose);
          if (distance < hover_event->distance)
            {
              closest = overlay;
              hover_event->distance = distance;
              graphene_matrix_init_from_matrix (&hover_event->pose, pose);
              graphene_point3d_init_from_point (&hover_event->point,
                                                &intersection_point);
            }
        }
    }

  HoverState *hover_state = &self->hover_state[controller_index];

  if (closest != NULL)
    {
      /* The recipient of the hover_end event should already see that this
       * overlay is not hovered anymore, so we need to set the hover state
       * before sending the event */
      OpenVROverlay *last_hovered_overlay = hover_state->overlay;
      hover_state->distance = hover_event->distance;
      hover_state->overlay = closest;
      graphene_matrix_init_from_matrix (&hover_state->pose, pose);

      /* We now hover over an overlay */
      if (closest != last_hovered_overlay
          && last_hovered_overlay != NULL)
        {
          OpenVRHoverEndEvent *hover_end_event =
              g_malloc (sizeof (OpenVRHoverEndEvent));
          hover_end_event->controller_index = controller_index;
          openvr_overlay_emit_hover_end (last_hovered_overlay, hover_end_event);
        }

      openvr_overlay_get_2d_offset (closest, &hover_event->point,
                                    &hover_state->intersection_offset);

      hover_event->controller_index = controller_index;
      openvr_overlay_emit_hover (closest, hover_event);
    }
  else
    {
      /* No intersection was found, nothing is hovered */
      g_free (hover_event);

      /* Emit no hover event only if we had hovered something earlier */
      if (hover_state->overlay != NULL)
        {
          OpenVROverlay *last_hovered_overlay = hover_state->overlay;
          hover_state->overlay = NULL;
          OpenVRHoverEndEvent *hover_end_event =
              g_malloc (sizeof (OpenVRHoverEndEvent));
          hover_end_event->controller_index = controller_index;
          openvr_overlay_emit_hover_end (last_hovered_overlay,
                                         hover_end_event);

          OpenVRNoHoverEvent *no_hover_event =
              g_malloc (sizeof (OpenVRNoHoverEvent));
          no_hover_event->controller_index = controller_index;
          g_signal_emit (self, overlay_manager_signals[NO_HOVER_EVENT], 0,
                         no_hover_event);
        }
    }
}

void
_drag_overlay (OpenVROverlayManager *self,
               graphene_matrix_t    *pose,
               int                   controller_index)
{
  HoverState *hover_state = &self->hover_state[controller_index];
  GrabState *grab_state = &self->grab_state[controller_index];

  graphene_vec3_t controller_translation;
  openvr_math_matrix_get_translation (pose, &controller_translation);
  graphene_point3d_t controller_translation_point;
  graphene_point3d_init_from_vec3 (&controller_translation_point,
                                   &controller_translation);
  graphene_quaternion_t controller_rotation;
  graphene_quaternion_init_from_matrix (&controller_rotation, pose);

  graphene_point3d_t distance_translation_point;
  graphene_point3d_init (&distance_translation_point,
                         0.f, 0.f, -hover_state->distance);


  graphene_matrix_t transformation_matrix;
  graphene_matrix_init_identity (&transformation_matrix);

  /* first translate the overlay so that the grab point is the origin */
  graphene_matrix_translate (&transformation_matrix,
                             &grab_state->offset_translation_point);

  /* then apply the rotation that the overlay had when it was grabbed */
  graphene_matrix_rotate_quaternion (
      &transformation_matrix, &grab_state->overlay_rotation);

  /* reverse the rotation induced by the controller pose when it was grabbed */
  graphene_matrix_rotate_quaternion (
      &transformation_matrix,
      &grab_state->overlay_transformed_rotation_neg);

  /* then translate the overlay to the controller ray distance */
  graphene_matrix_translate (&transformation_matrix,
                             &distance_translation_point);

  /* rotate the translated overlay. Because the original controller rotation has
   * been subtracted, this will only add the diff to the original rotation
   */
  graphene_matrix_rotate_quaternion (&transformation_matrix,
                                     &controller_rotation);

  /* and finally move the whole thing so the controller is the origin */
  graphene_matrix_translate (&transformation_matrix,
                             &controller_translation_point);

  openvr_overlay_set_transform_absolute (grab_state->overlay,
                                        &transformation_matrix);
}

void
openvr_overlay_manager_drag_start (OpenVROverlayManager *self,
                                   int                   controller_index)
{
  HoverState *hover_state = &self->hover_state[controller_index];
  GrabState *grab_state = &self->grab_state[controller_index];

  /* Copy hover to grab state */
  grab_state->overlay = hover_state->overlay;

  graphene_quaternion_t controller_rotation;
  graphene_quaternion_init_from_matrix (&controller_rotation,
                                        &hover_state->pose);

  graphene_matrix_t overlay_transform;
  openvr_overlay_get_transform_absolute (grab_state->overlay,
                                         &overlay_transform);
  graphene_quaternion_init_from_matrix (
      &grab_state->overlay_rotation, &overlay_transform);

  graphene_point3d_t distance_translation_point;
  graphene_point3d_init (&distance_translation_point,
                         0.f, 0.f, -hover_state->distance);

  graphene_point3d_t negative_distance_translation_point;
  graphene_point3d_init (&negative_distance_translation_point,
                         0.f, 0.f, +hover_state->distance);

  graphene_point3d_init (
      &grab_state->offset_translation_point,
      -hover_state->intersection_offset.x,
      -hover_state->intersection_offset.y,
      0.f);

  /* Calculate the inverse of the overlay rotatation that is induced by the
   * controller dragging the overlay in an arc to its current location when it
   * is grabbed. Multiplying this inverse rotation to the rotation of the
   * overlay will subtract the initial rotation induced by the controller pose
   * when the overlay was grabbed.
   */
  graphene_matrix_t target_transformation_matrix;
  graphene_matrix_init_identity (&target_transformation_matrix);
  graphene_matrix_translate (&target_transformation_matrix,
                             &distance_translation_point);
  graphene_matrix_rotate_quaternion (&target_transformation_matrix,
                                     &controller_rotation);
  graphene_matrix_translate (&target_transformation_matrix,
                             &negative_distance_translation_point);
  graphene_quaternion_t transformed_rotation;
  graphene_quaternion_init_from_matrix (&transformed_rotation,
                                        &target_transformation_matrix);
  graphene_quaternion_invert (
      &transformed_rotation,
      &grab_state->overlay_transformed_rotation_neg);
}

void
openvr_overlay_manager_check_grab (OpenVROverlayManager *self,
                                   int                   controller_index)
{
  HoverState *hover_state = &self->hover_state[controller_index];

  if (hover_state->overlay != NULL)
    {
      OpenVRGrabEvent *grab_event =
          g_malloc (sizeof (OpenVRGrabEvent));
      grab_event->controller_index = controller_index;
      openvr_overlay_emit_grab (hover_state->overlay, grab_event);
    }
}

void
openvr_overlay_manager_check_release (OpenVROverlayManager *self,
                                      int                   controller_index)
{
  GrabState *grab_state = &self->grab_state[controller_index];

  if (grab_state->overlay != NULL)
    {
      OpenVRReleaseEvent *release_event =
          g_malloc (sizeof (OpenVRReleaseEvent));
      release_event->controller_index = controller_index;
      openvr_overlay_emit_release (grab_state->overlay, release_event);
    }
  grab_state->overlay = NULL;
}

void
openvr_overlay_manager_update_pose (OpenVROverlayManager *self,
                                    graphene_matrix_t    *pose,
                                    int                   controller_index)
{
  /* Drag test */
  if (self->grab_state[controller_index].overlay != NULL)
    _drag_overlay (self, pose, controller_index);
  else
    _test_hover (self, pose, controller_index);
}

gboolean
openvr_overlay_manager_is_hovering (OpenVROverlayManager *self)
{
  for (uint32_t i = 0; i < OPENVR_CONTROLLER_COUNT; i++)
    if (self->hover_state[i].overlay != NULL)
      return TRUE;
  return FALSE;
}

gboolean
openvr_overlay_manager_is_grabbing (OpenVROverlayManager *self)
{
  for (uint32_t i = 0; i < OPENVR_CONTROLLER_COUNT; i++)
    if (self->grab_state[i].overlay != NULL)
      return TRUE;
  return FALSE;
}

