/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-controller.h"

#include <gdk/gdk.h>
#include "openvr-context.h"
#include "openvr_capi_global.h"

// sets openvr controller ids in the order openvr assigns them
gboolean
openvr_controller_init (OpenVRController *self, int num)
{
  OpenVRContext *context = openvr_context_get_instance ();

  // TODO: will steamvr give newly powered on controllers higher indices?
  // if not, this method could fail.
  int skipped = 0;
  for (uint32_t i = 0; i < k_unMaxTrackedDeviceCount; i++)
    {
      if (context->system->IsTrackedDeviceConnected (i))
        {
          ETrackedDeviceClass class =
              context->system->GetTrackedDeviceClass (i);
          if (class == ETrackedDeviceClass_TrackedDeviceClass_Controller)
            {
              // g_print("controller: %d: %d skipped %d\n", num, i,
              // skipped);
              if (skipped >= num)
                {
                  self->index = i;
                  g_print ("Controller %d: %d, skipped %d\n", num,
                           self->index, skipped);
                  self->initialized = TRUE;
                  break;
                }
              else
                {
                  skipped++;
                  continue;
                }
            }
        }
    }

  if (!self->initialized)
    return FALSE;

  return TRUE;
}

gboolean
_controller_to_ray (TrackedDevicePose_t *pose, graphene_ray_t *ray)
{
  graphene_matrix_t transform;
  if (!openvr_math_pose_to_matrix (pose, &transform))
    {
      return FALSE;
    }

  // now rotate it with the window orientation
  graphene_vec3_t direction;
  openvr_math_direction_from_matrix (&transform, &direction);

  graphene_vec3_t translation;
  openvr_math_vec3_init_from_matrix (&translation, &transform);

  graphene_ray_init_from_vec3 (ray, &translation, &direction);
  return TRUE;
}

gboolean
openvr_controller_intersect_overlay (OpenVRController   *self,
                                     OpenVROverlay      *overlay,
                                     graphene_point3d_t *intersection_point)
{
  // if controller is "too near", we don't have an intersection
  float epsilon = 0.00001;

  graphene_vec3_t overlay_translation;
  openvr_math_vec3_init_from_matrix (&overlay_translation, &overlay->transform);
  graphene_point3d_t overlay_origin;
  graphene_point3d_init_from_vec3 (&overlay_origin, &overlay_translation);

  // the normal of an overlay in xy plane faces the "backwards" direction +z
  graphene_vec3_t overlay_normal;
  graphene_vec3_init (&overlay_normal, 0, 0, 1);
  // now rotate the normal with the current overlay orientation
  openvr_math_direction_from_matrix_vec3 (&overlay->transform, &overlay_normal,
                                          &overlay_normal);

  graphene_plane_t plane;
  graphene_plane_init_from_point (&plane, &overlay_normal, &overlay_origin);

  OpenVRContext *context = openvr_context_get_instance ();

  TrackedDevicePose_t pose[k_unMaxTrackedDeviceCount];
  context->system->GetDeviceToAbsoluteTrackingPose (
    context->origin, 0, pose, k_unMaxTrackedDeviceCount);

  graphene_ray_t ray;
  _controller_to_ray (&pose[self->index], &ray);

  float dist = graphene_ray_get_distance_to_plane (&ray, &plane);
  if (dist == INFINITY)
    {
      // g_print("No intersection!\n");
      return FALSE;
    }
  if (dist < epsilon)
    {
      g_print ("Too near for intersection: %f < %f!\n", dist, epsilon);
      return FALSE;
    }

  graphene_ray_get_position_at (&ray, dist, intersection_point);

  /*
  g_print("intersection! %f %f %f\n",
    intersection_point->x,
    intersection_point->y,
    intersection_point->z
  );
  */

  return TRUE;
}

gboolean
openvr_controller_trigger_events (OpenVRController *self,
                                  OpenVROverlay    *overlay)
{
  TrackedDevicePose_t pose[k_unMaxTrackedDeviceCount];

  OpenVRContext *context = openvr_context_get_instance ();

  context->system->GetDeviceToAbsoluteTrackingPose (
    context->origin, 0, pose, k_unMaxTrackedDeviceCount);

  if (!self->initialized)
    {
      g_printerr ("trigger_events() with invalid controller called!\n");
      return FALSE;
    }

  graphene_matrix_t transform;
  if (!openvr_math_pose_to_matrix (&pose[self->index], &transform))
    return FALSE;

  // this is just a demo how to create your own event e.g. for sending
  // graphene data structures to a client application in this case it is the
  // transform matrix of the controller and a 3D intersection point, in case
  // the user is pointing at an overlay
  // TODO: note that we malloc() it here, so the client needs to free it
  struct _motion_event_3d *controller_pos_ev =
      malloc (sizeof (struct _motion_event_3d));
  graphene_matrix_init_from_matrix (&controller_pos_ev->transform, &transform);

  graphene_point3d_t intersection_point;
  gboolean intersects = openvr_overlay_intersects (overlay,
                                                  &intersection_point,
                                                  &transform);
  if (!intersects)
    {
      controller_pos_ev->has_intersection = FALSE;
      g_signal_emit (overlay, overlay_signals[MOTION_NOTIFY_EVENT3D], 0,
                     controller_pos_ev);

      // g_print("No intersection!\n");
      return FALSE;
    }
  else
    {
      controller_pos_ev->has_intersection = TRUE;
      graphene_point3d_init_from_point (&controller_pos_ev->intersection_point,
                                        &intersection_point);
      g_signal_emit (overlay, overlay_signals[MOTION_NOTIFY_EVENT3D], 0,
                     controller_pos_ev);

      // g_print("Intersects at point %f %f %f\n", intersection_point.x,
      //         intersection_point.y, intersection_point.z);
    }

  // GetOpenVRController is deprecated but simpler to use than actions
  VRControllerState_t controller_state;
  context->system->GetControllerState (self->index,
                                      &controller_state,
                                       sizeof (controller_state));

  // coordinates of the overlay in the VR space and coordinates of the
  // intersection point in the VR space can be used to calculate the position
  // of the intersection point relative to the overlay
  graphene_vec3_t overlay_translation;
  openvr_math_vec3_init_from_matrix (&overlay_translation, &overlay->transform);
  graphene_point3d_t overlay_position;
  graphene_point3d_init_from_vec3 (&overlay_position, &overlay_translation);

  // the transformation matrix describes the *center* point of an overlay
  // to calculate 2D coordinates relative to overlay origin we have to shift
  gfloat overlay_width;
  context->overlay->GetOverlayWidthInMeters (overlay->overlay_handle,
                                            &overlay_width);
  // there is no function to get the height or aspect ratio of an overlay
  // so we need to calculate it from width + texture size
  // the texture aspect ratio should be preserved
  uint32_t texture_width;
  uint32_t texture_height;
  context->overlay->GetOverlayTextureSize (overlay->overlay_handle,
                                          &texture_width,
                                          &texture_height);
  gfloat overlay_aspect = (float) texture_width / texture_height;
  gfloat overlay_height = overlay_width / overlay_aspect;
  /*
  g_print("Overlay width %f height %f aspect %f texture %dx%d\n",
          overlay_width, overlay_height, overlay_aspect,
          texture_width, texture_height);
  */

  // if the overlay is parallel to the xy plane the position in the overlay can
  // be easily calculated:
  /*
  gfloat in_overlay_x = intersection_point.x - overlay_position.x
                        + overlay_width / 2.f;
  gfloat in_overlay_y = intersection_point.y - overlay_position.y
                        + overlay_height / 2.f;
   */


  // to calculate the position in the overlay in any orientation, we can invert
  // the transformation matrix of the overlay. This transformation matrix would
  // bring the center of our overlay into the origin of the coordinate system,
  // facing us (+z), the overlay being in the xy plane (since by convention that
  // is the neutral position for overlays)
  //
  // since the transformation matrix transforms every possible point on the
  // overlay onto the same overlay as it is in the origin in the xy plane,
  // it transforms in particular the intersection point onto its position on the
  // overlay in the xy plane.
  // Then we only need to shift it by half of the overlay widht/height, because
  // the *center* of the overlay sits in the origin
  graphene_matrix_t inverted;
  graphene_matrix_inverse (&overlay->transform, &inverted);

  graphene_point3d_t transformed_intersection;
  graphene_matrix_transform_point3d (&inverted,
                                     &intersection_point,
                                     &transformed_intersection);
  gfloat in_overlay_x = transformed_intersection.x + overlay_width / 2.f;
  gfloat in_overlay_y = transformed_intersection.y + overlay_height / 2.f;
  /*
  g_print("transformed %f %f %f -> %f %f %f\n",
          intersection_point.x, intersection_point.y, intersection_point.z,
          in_overlay_x,
          in_overlay_y,
          transformed_intersection.z);
   */

  // notifies the client application where in the current overlay the user is
  // pointing at
  GdkEvent *event = gdk_event_new (GDK_MOTION_NOTIFY);
  event->motion.x = in_overlay_x;
  event->motion.y = in_overlay_y;
  g_signal_emit (overlay, overlay_signals[MOTION_NOTIFY_EVENT], 0, event);

  gboolean last_b1 = self->_button1_pressed;
  gboolean last_b2 = self->_button2_pressed;
  gboolean last_grip = self->_grip_pressed;

  if (controller_state.ulButtonPressed &
      ButtonMaskFromId (EVRButtonId_k_EButton_SteamVR_Trigger))
    {
      self->_button1_pressed = TRUE;
      if (!last_b1)
        {
          GdkEvent *event = gdk_event_new (GDK_BUTTON_PRESS);
          event->button.x = in_overlay_x;
          event->button.y = in_overlay_y;
          event->button.button = 1;
          g_signal_emit (overlay, overlay_signals[BUTTON_PRESS_EVENT], 0,
                         event);
          // g_print("%d: Pressed button 1\n",
          // self->index);
        }
    }
  else
    {
      self->_button1_pressed = FALSE;
      if (last_b1)
        {
          GdkEvent *event = gdk_event_new (GDK_BUTTON_RELEASE);
          event->button.x = in_overlay_x;
          event->button.y = in_overlay_y;
          event->button.button = 1;
          g_signal_emit (overlay, overlay_signals[BUTTON_RELEASE_EVENT], 0,
                         event);
          // g_print("%d: Released button 1\n",
          // self->index);
        }
    }

  if (controller_state.ulButtonPressed &
      ButtonMaskFromId (EVRButtonId_k_EButton_ApplicationMenu))
    {
      self->_button2_pressed = TRUE;
      if (!last_b2)
        {
          GdkEvent *event = gdk_event_new (GDK_BUTTON_PRESS);
          event->button.x = in_overlay_x;
          event->button.y = in_overlay_y;
          event->button.button = 2;
          g_signal_emit (overlay, overlay_signals[BUTTON_PRESS_EVENT], 0,
                         event);
          // g_print("%d: Pressed button 2\n",
          // state->index);
        }
    }
  else
    {
      self->_button2_pressed = FALSE;
      if (last_b2)
        {
          GdkEvent *event = gdk_event_new (GDK_BUTTON_RELEASE);
          event->button.x = in_overlay_x;
          event->button.y = in_overlay_y;
          event->button.button = 2;
          g_signal_emit (overlay, overlay_signals[BUTTON_RELEASE_EVENT], 0,
                         event);
          // g_print("%d: Released button 2\n",
          // self->index);
        }
    }

  if (controller_state.ulButtonPressed &
      ButtonMaskFromId (EVRButtonId_k_EButton_Grip))
    {
      self->_grip_pressed = TRUE;
      if (!last_grip)
        {
          GdkEvent *event = gdk_event_new (GDK_BUTTON_PRESS);
          event->button.x = in_overlay_x;
          event->button.y = in_overlay_y;
          event->button.button = 9;
          g_signal_emit (overlay, overlay_signals[BUTTON_PRESS_EVENT], 0,
                         event);
          // g_print("%d: Pressed grip button\n",
          // self->index);
        }
    }
  else
    {
      self->_grip_pressed = FALSE;
      if (last_grip)
        {
          GdkEvent *event = gdk_event_new (GDK_BUTTON_RELEASE);
          event->button.x = in_overlay_x;
          event->button.y = in_overlay_y;
          event->button.button = 9;
          g_signal_emit (overlay, overlay_signals[BUTTON_RELEASE_EVENT], 0,
                         event);
          // g_print("%d: Released grip button\n",
          // state->index);
        }
    }

  if (controller_state.ulButtonPressed &
      ButtonMaskFromId (EVRButtonId_k_EButton_Axis0))
    {
      GdkEvent *event = gdk_event_new (GDK_SCROLL);
      event->scroll.y = controller_state.rAxis[0].y;
      event->scroll.direction =
          controller_state.rAxis[0].y > 0 ? GDK_SCROLL_UP : GDK_SCROLL_DOWN;
      g_signal_emit (overlay, overlay_signals[SCROLL_EVENT], 0, event);
      // g_print("touchpad y %f\n", controller_state.rAxis[0].y);
    }

  return TRUE;
}
