/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-controller.h"

#include <gdk/gdk.h>
#include "openvr-context.h"
#include "openvr_capi_global.h"

G_DEFINE_TYPE (OpenVRController, openvr_controller, G_TYPE_OBJECT)

static void
openvr_controller_finalize (GObject *gobject);

static void
openvr_controller_class_init (OpenVRControllerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_controller_finalize;
}

static void
openvr_controller_init (OpenVRController *self)
{
  self->initialized = FALSE;
}

OpenVRController *
openvr_controller_new (void)
{
  return (OpenVRController*) g_object_new (OPENVR_TYPE_CONTROLLER, 0);
}

OpenVRController *
openvr_controller_new_from_id (uint32_t id)
{
  OpenVRController *controller = openvr_controller_new ();
  controller->index = id;
  controller->initialized = TRUE;
  return controller;
}

static void
openvr_controller_finalize (GObject *gobject)
{
  OpenVRController *self = OPENVR_CONTROLLER (gobject);
  (void) self;
}

GSList *
openvr_controller_enumerate ()
{
  OpenVRContext *context = openvr_context_get_instance ();

  GSList *controlllers = NULL;

  for (uint32_t i = 0; i < k_unMaxTrackedDeviceCount; i++)
    if (context->system->IsTrackedDeviceConnected (i))
      {
        ETrackedDeviceClass class = context->system->GetTrackedDeviceClass (i);
        if (class == ETrackedDeviceClass_TrackedDeviceClass_Controller)
          {
            g_print ("Controller %d found.\n", i);
            OpenVRController *controller = openvr_controller_new_from_id (i);
            controlllers = g_slist_append (controlllers, controller);
          }
      }

  g_print ("Found %d controllers.\n", g_slist_length (controlllers));

  return controlllers;
}

#if 0
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
#endif

gboolean
openvr_controller_poll_event (OpenVRController *self,
                              OpenVROverlay    *overlay)
{
  if (!self->initialized)
    {
      g_printerr ("openvr_controller_poll_event()"
                  " called with invalid controller!\n");
      return FALSE;
    }

  TrackedDevicePose_t pose[k_unMaxTrackedDeviceCount];

  OpenVRContext *context = openvr_context_get_instance ();

  context->system->GetDeviceToAbsoluteTrackingPose (
    context->origin, 0, pose, k_unMaxTrackedDeviceCount);

  graphene_matrix_t transform;
  if (!openvr_math_pose_to_matrix (&pose[self->index], &transform))
    return FALSE;

  /*
   * This shows how to create a custom event, e.g. for sending
   * graphene data structures to a client application in this case it is the
   * transform matrix of the controller and a 3D intersection point, in case
   * the user is pointing at an overlay.
   * mote that we malloc() it here, so the client needs to free it.
   */

  OpenVRController3DEvent *event_3d = malloc (sizeof (OpenVRController3DEvent));
  graphene_matrix_init_from_matrix (&event_3d->transform, &transform);

  graphene_point3d_t intersection_point;
  gboolean intersects = openvr_overlay_intersects (overlay,
                                                  &intersection_point,
                                                  &transform);
  if (!intersects)
    {
      event_3d->has_intersection = FALSE;
      g_signal_emit (overlay, overlay_signals[MOTION_NOTIFY_EVENT3D], 0,
                     event_3d);

      // g_print("No intersection!\n");
      return FALSE;
    }
  else
    {
      event_3d->has_intersection = TRUE;
      graphene_point3d_init_from_point (&event_3d->intersection_point,
                                        &intersection_point);
      g_signal_emit (overlay, overlay_signals[MOTION_NOTIFY_EVENT3D], 0,
                     event_3d);

      // g_print("Intersects at point %f %f %f\n", intersection_point.x,
      //         intersection_point.y, intersection_point.z);
    }

  /* GetOpenVRController is deprecated but simpler to use than actions */
  VRControllerState_t controller_state;
  context->system->GetControllerState (self->index,
                                      &controller_state,
                                       sizeof (controller_state));

  /*
   * Coordinates of the overlay in the VR space and coordinates of the
   * intersection point in the VR space can be used to calculate the position
   * of the intersection point relative to the overlay.
   */

  graphene_vec3_t overlay_translation;
  openvr_math_vec3_init_from_matrix (&overlay_translation, &overlay->transform);
  graphene_point3d_t overlay_position;
  graphene_point3d_init_from_vec3 (&overlay_position, &overlay_translation);

  /*
   * The transformation matrix describes the *center* point of an overlay
   * to calculate 2D coordinates relative to overlay origin we have to shift.
   */

  gfloat overlay_width;
  context->overlay->GetOverlayWidthInMeters (overlay->overlay_handle,
                                            &overlay_width);

  /*
   * There is no function to get the height or aspect ratio of an overlay
   * so we need to calculate it from width + texture size
   * the texture aspect ratio should be preserved.
   */

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

  /*
   * If the overlay is parallel to the xy plane the position in the overlay can
   * be easily calculated:
   */
  /*
  gfloat in_overlay_x = intersection_point.x - overlay_position.x
                        + overlay_width / 2.f;
  gfloat in_overlay_y = intersection_point.y - overlay_position.y
                        + overlay_height / 2.f;
   */

  /*
   * To calculate the position in the overlay in any orientation, we can invert
   * the transformation matrix of the overlay. This transformation matrix would
   * bring the center of our overlay into the origin of the coordinate system,
   * facing us (+z), the overlay being in the xy plane (since by convention that
   * is the neutral position for overlays).
   *
   * Since the transformation matrix transforms every possible point on the
   * overlay onto the same overlay as it is in the origin in the xy plane,
   * it transforms in particular the intersection point onto its position on the
   * overlay in the xy plane.
   * Then we only need to shift it by half of the overlay widht/height, because
   * the *center* of the overlay sits in the origin.
   */

  graphene_matrix_t inverted;
  graphene_matrix_inverse (&overlay->transform, &inverted);

  graphene_point3d_t transformed_intersection;
  graphene_matrix_transform_point3d (&inverted,
                                     &intersection_point,
                                     &transformed_intersection);
  gfloat in_overlay_x = transformed_intersection.x + overlay_width / 2.f;
  gfloat in_overlay_y = transformed_intersection.y + overlay_height / 2.f;
  /*
  g_print ("transformed %f %f %f -> %f %f %f\n",
           intersection_point.x, intersection_point.y, intersection_point.z,
           in_overlay_x,
           in_overlay_y,
           transformed_intersection.z);
  */

  /*
   * Notifies the client application where in the current overlay the user is
   * pointing at
   */

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
