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

  VRControllerState_t state;
  OpenVRContext *context = openvr_context_get_instance ();
  if (!context->system->GetControllerState (controller->index,
                                           &state, sizeof (state)))
    g_printerr ("GetControllerState returned error.\n");
  else
    controller->last_pressed_state = state.ulButtonPressed;

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
openvr_controller_get_transformation (OpenVRController  *self,
                                      graphene_matrix_t *transform)
{
  TrackedDevicePose_t pose[k_unMaxTrackedDeviceCount];

  OpenVRContext *context = openvr_context_get_instance ();

  /* TODO: Can we maybe get less than all tracked device poses
   *       from the API here? Most likely GetControllerStateWithPose. */
  context->system->GetDeviceToAbsoluteTrackingPose (
    context->origin, 0, pose, k_unMaxTrackedDeviceCount);

  if (!openvr_math_pose_to_matrix (&pose[self->index], transform))
    return FALSE;

  return TRUE;
}

/*
 * This shows how to create a custom event, e.g. for sending
 * graphene data structures to a client application in this case it is the
 * transform matrix of the controller and a 3D intersection point, in case
 * the user is pointing at an overlay.
 * mote that we malloc() it here, so the client needs to free it.
 */

void
_emit_3d_event (OpenVROverlay      *overlay,
                gboolean            intersects,
                graphene_matrix_t  *transform,
                graphene_point3d_t *intersection_point)
{
  OpenVRController3DEvent *event_3d = malloc (sizeof (OpenVRController3DEvent));
  graphene_matrix_init_from_matrix (&event_3d->transform, transform);

  event_3d->has_intersection = intersects;

  if (intersects)
    {
      graphene_point3d_init_from_point (&event_3d->intersection_point,
                                        intersection_point);

      // g_print("Intersects at point %f %f %f\n", intersection_point.x,
      //         intersection_point.y, intersection_point.z);
    }

  g_signal_emit (overlay, overlay_signals[MOTION_NOTIFY_EVENT3D], 0, event_3d);
}

GdkEvent *
_create_positioned_button_event (GdkEventType type,
                                 graphene_vec2_t *position_2d)
{
  GdkEvent *event = gdk_event_new (type);
  event->button.x = graphene_vec2_get_x (position_2d);
  event->button.y = graphene_vec2_get_y (position_2d);
  return event;
}

uint64_t
_is_pressed (uint64_t state, EVRButtonId id)
{
  return state & ButtonMaskFromId (id);
}

EVRButtonId
_map_button (OpenVRButton button)
{
  switch (button)
    {
      case OPENVR_BUTTON_TRIGGER: return EVRButtonId_k_EButton_SteamVR_Trigger;
      case OPENVR_BUTTON_MENU:    return EVRButtonId_k_EButton_ApplicationMenu;
      case OPENVR_BUTTON_GRIP:    return EVRButtonId_k_EButton_Grip;
      case OPENVR_BUTTON_AXIS0:   return EVRButtonId_k_EButton_Axis0;
      default:
        g_printerr ("Unknown OpenVRButton %d\n", button);
    }
  return 0;
}

void
_check_press_release (OpenVRController *self,
                      OpenVROverlay    *overlay,
                      uint64_t          pressed_state,
                      OpenVRButton      button,
                      graphene_vec2_t  *position_2d)
{
  EVRButtonId button_id = _map_button (button);

  uint64_t is_pressed = _is_pressed (pressed_state, button_id);
  uint64_t was_pressed = _is_pressed (self->last_pressed_state, button_id);

  if (is_pressed != was_pressed)
    {
      GdkEventType type;
      guint signal;
      if (is_pressed)
        {
          type = GDK_BUTTON_PRESS;
          signal = BUTTON_PRESS_EVENT;
        }
      else
        {
          type = GDK_BUTTON_RELEASE;
          signal = BUTTON_RELEASE_EVENT;
        }

      GdkEvent *event = _create_positioned_button_event (type, position_2d);
      event->button.button = button;
      g_signal_emit (overlay, overlay_signals[signal], 0, event);
      // g_print ("Controller #%d: %d button %d\n", self->index, type, button);
    }
}

void
_emit_overlay_events (OpenVRController    *self,
                      OpenVROverlay       *overlay,
                      VRControllerState_t *state,
                      graphene_vec2_t     *position_2d)
{
  /* Motion event */
  GdkEvent *event = gdk_event_new (GDK_MOTION_NOTIFY);
  event->motion.x = graphene_vec2_get_x (position_2d);
  event->motion.y = graphene_vec2_get_y (position_2d);
  g_signal_emit (overlay, overlay_signals[MOTION_NOTIFY_EVENT], 0, event);

  /* Press / release events */
  uint64_t pressed_state = state->ulButtonPressed;
  _check_press_release (self, overlay, pressed_state,
                        OPENVR_BUTTON_TRIGGER, position_2d);
  _check_press_release (self, overlay, pressed_state,
                        OPENVR_BUTTON_MENU, position_2d);
  _check_press_release (self, overlay, pressed_state,
                        OPENVR_BUTTON_GRIP, position_2d);

  /* Scroll event from touchpad */
  if (_is_pressed (pressed_state, EVRButtonId_k_EButton_Axis0))
    {
      GdkEvent *event = gdk_event_new (GDK_SCROLL);
      event->scroll.y = state->rAxis[0].y;
      event->scroll.direction =
        state->rAxis[0].y > 0 ? GDK_SCROLL_UP : GDK_SCROLL_DOWN;
      g_signal_emit (overlay, overlay_signals[SCROLL_EVENT], 0, event);
      // g_print ("touchpad x %f y %f\n",
      //          state->rAxis[0].x,
      //          state->rAxis[0].y);
    }

  self->last_pressed_state = pressed_state;
}

gboolean
openvr_controller_poll_overlay_event (OpenVRController *self,
                                      OpenVROverlay    *overlay)
{
  if (!self->initialized)
    {
      g_printerr ("openvr_controller_poll_event()"
                  " called with invalid controller!\n");
      return FALSE;
    }

  graphene_matrix_t transform;
  openvr_controller_get_transformation (self, &transform);

  graphene_point3d_t intersection_point;
  gboolean intersects = openvr_overlay_intersects (overlay,
                                                  &intersection_point,
                                                  &transform);

  _emit_3d_event (overlay, intersects, &transform, &intersection_point);

  if (!intersects)
    return FALSE;

  /* GetOpenVRController is deprecated but simpler to use than actions */
  OpenVRContext *context = openvr_context_get_instance ();
  VRControllerState_t controller_state;
  if (!context->system->GetControllerState (self->index,
                                           &controller_state,
                                            sizeof (controller_state)))
    {
      g_printerr ("GetControllerState returned error.\n");
      return FALSE;
    }

  graphene_vec2_t position_2d;
  if (!openvr_overlay_get_2d_intersection (overlay,
                                          &intersection_point,
                                          &position_2d))
    return FALSE;

  _emit_overlay_events (self, overlay, &controller_state, &position_2d);

  return TRUE;
}
