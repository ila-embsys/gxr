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

enum {
  MOTION_3D_NOTIFY_EVENT,
  BUTTON_PRESS_EVENT,
  BUTTON_RELEASE_EVENT,
  TOUCHPAD_EVENT,
  CONNECTION_EVENT,
  LAST_SIGNAL
};

static guint controller_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (OpenVRController, openvr_controller, G_TYPE_OBJECT)

static void
openvr_controller_finalize (GObject *gobject);

static void
openvr_controller_class_init (OpenVRControllerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  controller_signals[MOTION_3D_NOTIFY_EVENT] =
    g_signal_new ("motion-3d-notify-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  controller_signals[BUTTON_PRESS_EVENT] =
    g_signal_new ("button-press-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  controller_signals[BUTTON_RELEASE_EVENT] =
    g_signal_new ("button-release-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  controller_signals[TOUCHPAD_EVENT] =
    g_signal_new ("touchpad-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  controller_signals[CONNECTION_EVENT] =
    g_signal_new ("connection-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

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

uint64_t
openvr_controller_is_pressed (uint64_t state, EVRButtonId id)
{
  return state & ButtonMaskFromId (id);
}

EVRButtonId
openvr_controller_to_evr_button (OpenVRButton button)
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
_emit_motion_3d_event (OpenVRController *self, graphene_matrix_t *transform)
{
  OpenVRMotion3DEvent *event = malloc (sizeof (OpenVRMotion3DEvent));
  graphene_matrix_init_from_matrix (&event->transform, transform);
  g_signal_emit (self, controller_signals[MOTION_3D_NOTIFY_EVENT], 0, event);
}

void
_controller_check_press_release (OpenVRController *self,
                                 uint64_t          pressed_state,
                                 OpenVRButton      button)
{
  EVRButtonId button_id = openvr_controller_to_evr_button (button);

  uint64_t is_pressed = openvr_controller_is_pressed (pressed_state, button_id);
  uint64_t was_pressed =
    openvr_controller_is_pressed (self->last_pressed_state, button_id);

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

      GdkEvent *event = gdk_event_new (type);
      event->button.button = button;
      g_signal_emit (self, controller_signals[signal], 0, event);
    }
}

gboolean
openvr_controller_poll_event (OpenVRController *self)
{
  if (!self->initialized)
    {
      g_printerr ("openvr_controller_poll_event()"
                  " called with invalid controller!\n");
      return FALSE;
    }

  graphene_matrix_t transform;
  openvr_controller_get_transformation (self, &transform);

  _emit_motion_3d_event (self, &transform);

  /* GetOpenVRController is deprecated but simpler to use than actions */
  OpenVRContext *context = openvr_context_get_instance ();
  VRControllerState_t state;
  if (!context->system->GetControllerState (self->index,
                                           &state, sizeof (state)))
    {
      g_printerr ("GetControllerState returned error.\n");
      return FALSE;
    }

  /* Press / release events */
  uint64_t pressed_state = state.ulButtonPressed;
  _controller_check_press_release (self, pressed_state, OPENVR_BUTTON_TRIGGER);
  _controller_check_press_release (self, pressed_state, OPENVR_BUTTON_MENU);
  _controller_check_press_release (self, pressed_state, OPENVR_BUTTON_GRIP);

  /* Scroll event from touchpad */
#if 0
  if (openvr_controller_is_pressed (pressed_state, EVRButtonId_k_EButton_Axis0))
    {
      GdkEvent *event = gdk_event_new (GDK_SCROLL);
      event->scroll.y = state.rAxis[0].y;
      event->scroll.direction =
        state.rAxis[0].y > 0 ? GDK_SCROLL_UP : GDK_SCROLL_DOWN;
      g_signal_emit (self, controller_signals[SCROLL_EVENT], 0, event);
      // g_print ("touchpad x %f y %f\n",
      //          state->rAxis[0].x,
      //          state->rAxis[0].y);
    }
#endif

  self->last_pressed_state = pressed_state;

  return TRUE;
}
