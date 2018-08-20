/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-controller.h"

#include <gdk/gdk.h>
#include "openvr-context.h"

// missing from openvr_capi.h
uint64_t ButtonMaskFromId (EVRButtonId id)
{
  return 1ull << id;
}

// sets openvr controller ids in the order openvr assigns them
gboolean
init_controller (ControllerState *state, int num)
{
  OpenVRContext *context = openvr_context_get_instance ();

  // TODO: will steamvr give newly powered on controllers higher indices?
  // if not, this method could fail.
  int skipped = 0;
  for (uint32_t dev_index = 0; dev_index < k_unMaxTrackedDeviceCount; dev_index++)
    {
      if (context->system->IsTrackedDeviceConnected (dev_index))
        {
          ETrackedDeviceClass class =
              context->system->GetTrackedDeviceClass (dev_index);
          if (class == ETrackedDeviceClass_TrackedDeviceClass_Controller)
            {
              // g_print("controller: %d: %d skipped %d\n", num, dev_index,
              // skipped);
              if (skipped >= num)
                {
                  state->unControllerDeviceIndex = dev_index;
                  g_print ("Controller %d: %d, skipped %d\n", num,
                           state->unControllerDeviceIndex, skipped);
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

  if (!state->initialized)
    return FALSE;

  return TRUE;
}

gboolean
graphene_direction_from_matrix_vec3 (graphene_matrix_t *matrix,
                                     graphene_vec3_t   *start,
                                     graphene_vec3_t   *direction)
{
  graphene_quaternion_t orientation;
  graphene_quaternion_init_from_matrix (&orientation, matrix);

  // construct a matrix that only contains rotation instead of
  // rotation + translation like transform
  graphene_matrix_t rotation_matrix;
  graphene_matrix_init_identity (&rotation_matrix);
  graphene_matrix_rotate_quaternion (&rotation_matrix, &orientation);

  graphene_matrix_transform_vec3 (&rotation_matrix, start, direction);
  return TRUE;
}

gboolean
graphene_direction_from_matrix (graphene_matrix_t *matrix,
                                graphene_vec3_t   *direction)
{
  // in openvr, the neutral forward direction is along the -z axis
  graphene_vec3_t start;
  graphene_vec3_init (&start, 0, 0, -1);

  return graphene_direction_from_matrix_vec3 (matrix, &start, direction);
}

gboolean
openvr_overlay_intersect (OpenVROverlay      *overlay,
                          graphene_point3d_t *intersection_point,
                          graphene_matrix_t  *transform)
{
  OpenVRContext *context = openvr_context_get_instance ();
  VROverlayIntersectionParams_t params;
  params.eOrigin = context->origin;

  graphene_vec3_t direction;
  graphene_direction_from_matrix (transform, &direction);

  params.vSource.v[0] = graphene_matrix_get_value (transform, 3, 0);
  params.vSource.v[1] = graphene_matrix_get_value (transform, 3, 1);
  params.vSource.v[2] = graphene_matrix_get_value (transform, 3, 2);

  params.vDirection.v[0] = graphene_vec3_get_x (&direction);
  params.vDirection.v[1] = graphene_vec3_get_y (&direction);
  params.vDirection.v[2] = graphene_vec3_get_z (&direction);

  // g_print("Controller position: %f %f %f    Controller direction: %f %f
  // %f\n",
  //        params.vSource.v[0], params.vSource.v[1], params.vSource.v[2],
  //        params.vDirection.v[0], params.vDirection.v[1],
  //        params.vDirection.v[2]);

  struct VROverlayIntersectionResults_t results;
  gboolean intersects = context->overlay->ComputeOverlayIntersection (
      overlay->overlay_handle, &params, &results);

  intersection_point->x = results.vPoint.v[0];
  intersection_point->y = results.vPoint.v[1];
  intersection_point->z = results.vPoint.v[2];
  return intersects;
}

// translation vector
gboolean
graphene_vec3_init_from_matrix (graphene_vec3_t *vec, graphene_matrix_t *matrix)
{
  graphene_vec3_init (vec,
                      graphene_matrix_get_value (matrix, 3, 0),
                      graphene_matrix_get_value (matrix, 3, 1),
                      graphene_matrix_get_value (matrix, 3, 2));
  return TRUE;
}

gboolean
openvr_to_graphene_matrix (TrackedDevicePose_t *pose,
                           graphene_matrix_t   *transform)
{
  if (pose->eTrackingResult !=
      ETrackingResult_TrackingResult_Running_OK)
    {
      // g_print("Tracking result: Not running ok: %d!\n",
      //  pose->eTrackingResult);
      return FALSE;
    }
  else if (pose->bPoseIsValid)
    {
      float mat4x4[16] = {
        pose->mDeviceToAbsoluteTracking.m[0][0],
        pose->mDeviceToAbsoluteTracking.m[1][0],
        pose->mDeviceToAbsoluteTracking.m[2][0],
        0,
        pose->mDeviceToAbsoluteTracking.m[0][1],
        pose->mDeviceToAbsoluteTracking.m[1][1],
        pose->mDeviceToAbsoluteTracking.m[2][1],
        0,
        pose->mDeviceToAbsoluteTracking.m[0][2],
        pose->mDeviceToAbsoluteTracking.m[1][2],
        pose->mDeviceToAbsoluteTracking.m[2][2],
        0,
        pose->mDeviceToAbsoluteTracking.m[0][3],
        pose->mDeviceToAbsoluteTracking.m[1][3],
        pose->mDeviceToAbsoluteTracking.m[2][3],
        1
      };

      graphene_matrix_init_from_float (transform, mat4x4);

      return TRUE;
    }
  else
    {
      // g_print("controller: Pose invalid\n");
      return FALSE;
    }
}

gboolean
openvr_controller_to_ray (TrackedDevicePose_t *pose, graphene_ray_t *ray)
{
  graphene_matrix_t transform;
  if (!openvr_to_graphene_matrix (pose, &transform))
    {
      return FALSE;
    }

  // now rotate it with the window orientation
  graphene_vec3_t direction;
  graphene_direction_from_matrix (&transform, &direction);

  graphene_vec3_t translation;
  graphene_vec3_init_from_matrix (&translation, &transform);

  graphene_ray_init_from_vec3 (ray, &translation, &direction);
  return TRUE;
}

gboolean
openvr_overlay_plane_intersect (ControllerState    *state,
                                OpenVROverlay      *overlay,
                                graphene_point3d_t *intersection_point)
{
  // if controller is "too near", we don't have an intersection
  float epsilon = 0.00001;

  graphene_vec3_t overlay_translation;
  graphene_vec3_init_from_matrix (&overlay_translation, &overlay->transform);
  graphene_point3d_t overlay_origin;
  graphene_point3d_init_from_vec3 (&overlay_origin, &overlay_translation);

  // the normal of an overlay in xy plane faces the "backwards" direction +z
  graphene_vec3_t overlay_normal;
  graphene_vec3_init (&overlay_normal, 0, 0, 1);
  // now rotate the normal with the current overlay orientation
  graphene_direction_from_matrix_vec3 (&overlay->transform, &overlay_normal,
                                       &overlay_normal);

  graphene_plane_t plane;
  graphene_plane_init_from_point (&plane, &overlay_normal, &overlay_origin);

  OpenVRContext *context = openvr_context_get_instance ();

  TrackedDevicePose_t pTrackedDevicePose[k_unMaxTrackedDeviceCount];
  context->system->GetDeviceToAbsoluteTrackingPose (
    context->origin,
    0, pTrackedDevicePose,
    k_unMaxTrackedDeviceCount);

  graphene_ray_t ray;
  openvr_controller_to_ray (&pTrackedDevicePose[state->unControllerDeviceIndex],
                            &ray);

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
trigger_events (ControllerState *state, OpenVROverlay *overlay)
{
  TrackedDevicePose_t pTrackedDevicePose[k_unMaxTrackedDeviceCount];

  OpenVRContext *context = openvr_context_get_instance ();

  context->system->GetDeviceToAbsoluteTrackingPose (
    context->origin,
    0, pTrackedDevicePose,
    k_unMaxTrackedDeviceCount);

  if (!state->initialized)
    {
      g_printerr ("trigger_events() with invalid controller called!\n");
      return FALSE;
    }

  graphene_matrix_t transform;
  if (!openvr_to_graphene_matrix (
          &pTrackedDevicePose[state->unControllerDeviceIndex], &transform))
    {
      return FALSE;
    }

  // this is just a demo how to create your own event e.g. for sending
  // graphene data structures to a client application in this case it is the
  // transform matrix of the controller and a 3D intersection point, in case
  // the user is pointing at an overlay
  // TODO: note that we malloc() it here, so the client needs to free it
  struct _motion_event_3d *controller_pos_ev =
      malloc (sizeof (struct _motion_event_3d));
  graphene_matrix_init_from_matrix (&controller_pos_ev->transform, &transform);

  graphene_point3d_t intersection_point;
  gboolean           intersects =
      openvr_overlay_intersect (overlay, &intersection_point, &transform);
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

  // GetControllerState is deprecated but simpler to use than actions
  VRControllerState_t controller_state;
  context->system->GetControllerState (state->unControllerDeviceIndex,
                                      &controller_state,
                                       sizeof (controller_state));

  // coordinates of the overlay in the VR space and coordinates of the
  // intersection point in the VR space can be used to calculate the position
  // of the intersection point relative to the overlay
  graphene_vec3_t overlay_translation;
  graphene_vec3_init_from_matrix (&overlay_translation, &overlay->transform);
  graphene_point3d_t overlay_position;
  graphene_point3d_init_from_vec3 (&overlay_position, &overlay_translation);

  // TODO: overlay can be any orientation so this is only correct for overlays
  // in the xy plane.
  // implement method that translates window to origin & xy plane
  // then these operations become trivial

  // the transformation matrix describes the *center* point of an overlay
  // to calculate 2D coordinates relative to overlay origin we have to shift
  gfloat overlay_width;
  context->overlay->GetOverlayWidthInMeters(overlay->overlay_handle,
                                           &overlay_width);
  // there is no function to get the height or aspect ratio of an overlay
  // so we need to calculate it from width + texture size
  // the texture aspect ratio should be preserved
  uint32_t texture_width;
  uint32_t texture_height;
  context->overlay->GetOverlayTextureSize(overlay->overlay_handle,
                                         &texture_width,
                                         &texture_height);
  gfloat overlay_aspect = (float) texture_width / texture_height;
  gfloat overlay_height = overlay_width / overlay_aspect;
  /*
  g_print("Overlay width %f height %f aspect %f texture %dx%d\n",
          overlay_width, overlay_height, overlay_aspect,
          texture_width, texture_height);
  */
  gfloat in_overlay_x = intersection_point.x - overlay_position.x
                        + overlay_width / 2.f;
  gfloat in_overlay_y = intersection_point.y - overlay_position.y
                        + overlay_height / 2.f;

  // notifies the client application where in the current overlay the user is
  // pointing at
  GdkEvent *event = gdk_event_new (GDK_MOTION_NOTIFY);
  event->motion.x = in_overlay_x;
  event->motion.y = in_overlay_y;
  g_signal_emit (overlay, overlay_signals[MOTION_NOTIFY_EVENT], 0, event);

  gboolean last_b1 = state->_button1_pressed;
  gboolean last_b2 = state->_button2_pressed;
  gboolean last_grip = state->_grip_pressed;

  if (controller_state.ulButtonPressed &
      ButtonMaskFromId (EVRButtonId_k_EButton_SteamVR_Trigger))
    {
      state->_button1_pressed = TRUE;
      if (!last_b1)
        {
          GdkEvent *event = gdk_event_new (GDK_BUTTON_PRESS);
          event->button.x = in_overlay_x;
          event->button.y = in_overlay_y;
          event->button.button = 1;
          g_signal_emit (overlay, overlay_signals[BUTTON_PRESS_EVENT], 0,
                         event);
          // g_print("%d: Pressed button 1\n",
          // state->unControllerDeviceIndex);
        }
    }
  else
    {
      state->_button1_pressed = FALSE;
      if (last_b1)
        {
          GdkEvent *event = gdk_event_new (GDK_BUTTON_RELEASE);
          event->button.x = in_overlay_x;
          event->button.y = in_overlay_y;
          event->button.button = 1;
          g_signal_emit (overlay, overlay_signals[BUTTON_RELEASE_EVENT], 0,
                         event);
          // g_print("%d: Released button 1\n",
          // state->unControllerDeviceIndex);
        }
    }

  if (controller_state.ulButtonPressed &
      ButtonMaskFromId (EVRButtonId_k_EButton_ApplicationMenu))
    {
      state->_button2_pressed = TRUE;
      if (!last_b2)
        {
          GdkEvent *event = gdk_event_new (GDK_BUTTON_PRESS);
          event->button.x = in_overlay_x;
          event->button.y = in_overlay_y;
          event->button.button = 2;
          g_signal_emit (overlay, overlay_signals[BUTTON_PRESS_EVENT], 0,
                         event);
          // g_print("%d: Pressed button 2\n",
          // state->unControllerDeviceIndex);
        }
    }
  else
    {
      state->_button2_pressed = FALSE;
      if (last_b2)
        {
          GdkEvent *event = gdk_event_new (GDK_BUTTON_RELEASE);
          event->button.x = in_overlay_x;
          event->button.y = in_overlay_y;
          event->button.button = 2;
          g_signal_emit (overlay, overlay_signals[BUTTON_RELEASE_EVENT], 0,
                         event);
          // g_print("%d: Released button 2\n",
          // state->unControllerDeviceIndex);
        }
    }

  if (controller_state.ulButtonPressed &
      ButtonMaskFromId (EVRButtonId_k_EButton_Grip))
    {
      state->_grip_pressed = TRUE;
      if (!last_grip)
        {
          GdkEvent *event = gdk_event_new (GDK_BUTTON_PRESS);
          event->button.x = in_overlay_x;
          event->button.y = in_overlay_y;
          event->button.button = 9;
          g_signal_emit (overlay, overlay_signals[BUTTON_PRESS_EVENT], 0,
                         event);
          // g_print("%d: Pressed grip button\n",
          // state->unControllerDeviceIndex);
        }
    }
  else
    {
      state->_grip_pressed = FALSE;
      if (last_grip)
        {
          GdkEvent *event = gdk_event_new (GDK_BUTTON_RELEASE);
          event->button.x = in_overlay_x;
          event->button.y = in_overlay_y;
          event->button.button = 9;
          g_signal_emit (overlay, overlay_signals[BUTTON_RELEASE_EVENT], 0,
                         event);
          // g_print("%d: Released grip button\n",
          // state->unControllerDeviceIndex);
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
