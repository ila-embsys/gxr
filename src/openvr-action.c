/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gdk/gdk.h>

#include "openvr-action.h"

#include "openvr-context.h"
#include "openvr-math.h"

struct _OpenVRAction
{
  GObject parent;

  VRActionHandle_t handle;

  OpenVRActionType type;
};

G_DEFINE_TYPE (OpenVRAction, openvr_action, G_TYPE_OBJECT)

enum {
  DIGITAL_EVENT,
  ANALOG_EVENT,
  POSE_EVENT,
  LAST_SIGNAL
};

static guint action_signals[LAST_SIGNAL] = { 0 };

gboolean
openvr_action_load_manifest (char *path)
{
  OpenVRContext *context = openvr_context_get_instance ();

  EVRInputError err;
  err = context->input->SetActionManifestPath (path);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: SetActionManifestPath: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }
  return TRUE;
}

static void
openvr_action_finalize (GObject *gobject);

gboolean
openvr_action_load_handle (OpenVRAction *self,
                           char         *url);

static void
openvr_action_class_init (OpenVRActionClass *klass)
{
  action_signals[DIGITAL_EVENT] =
    g_signal_new ("digital-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  action_signals[ANALOG_EVENT] =
    g_signal_new ("analog-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  action_signals[POSE_EVENT] =
    g_signal_new ("pose-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_action_finalize;
}

static void
openvr_action_init (OpenVRAction *self)
{
  self->handle = k_ulInvalidActionSetHandle;
}

OpenVRAction *
openvr_action_new (void)
{
  return (OpenVRAction*) g_object_new (OPENVR_TYPE_ACTION, 0);
}

OpenVRAction *
openvr_action_new_from_url (char *url)
{
  OpenVRAction *self = openvr_action_new ();
  if (!openvr_action_load_handle (self, url))
    {
      g_object_unref (self);
      self = NULL;
    }
  return self;
}

OpenVRAction *
openvr_action_new_from_type_url (OpenVRActionType type, char *url)
{
  OpenVRAction *self = openvr_action_new ();
  self->type = type;
  if (!openvr_action_load_handle (self, url))
    {
      g_object_unref (self);
      self = NULL;
    }
  return self;
}

gboolean
openvr_action_load_handle (OpenVRAction *self,
                           char         *url)
{
  OpenVRContext *context = openvr_context_get_instance ();

  EVRInputError err;
  err = context->input->GetActionHandle (url, &self->handle);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: GetActionHandle: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }
  return TRUE;
}

gboolean
openvr_action_poll_digital (OpenVRAction *self)
{
  OpenVRContext *context = openvr_context_get_instance ();

  InputDigitalActionData_t data;

  EVRInputError err;
  err = context->input->GetDigitalActionData (self->handle, &data,
                                              sizeof(data),
                                              k_ulInvalidInputValueHandle);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: GetDigitalActionData: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }

  OpenVRDigitalEvent *event = g_malloc (sizeof (OpenVRDigitalEvent));
  event->active = data.bActive;
  event->state = data.bState;
  event->changed = data.bChanged;
  event->time = data.fUpdateTime;

  g_signal_emit (self, action_signals[DIGITAL_EVENT], 0, event);

  return TRUE;
}

gboolean
openvr_action_poll (OpenVRAction *self)
{
  switch (self->type)
    {
    case OPENVR_ACTION_DIGITAL:
      return openvr_action_poll_digital (self);
    case OPENVR_ACTION_ANALOG:
      return openvr_action_poll_analog (self);
    case OPENVR_ACTION_POSE:
      return openvr_action_poll_pose (self);
    default:
      g_printerr ("Uknown action type %d\n", self->type);
      return FALSE;
    }
}


gboolean
openvr_action_poll_analog (OpenVRAction *self)
{
  OpenVRContext *context = openvr_context_get_instance ();

  InputAnalogActionData_t data;

  EVRInputError err;
  err = context->input->GetAnalogActionData (self->handle, &data,
                                             sizeof(data),
                                             k_ulInvalidInputValueHandle);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: GetAnalogActionData: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }

  OpenVRAnalogEvent *event = g_malloc (sizeof (OpenVRAnalogEvent));
  event->active = data.bActive;
  graphene_vec3_init (&event->state, data.x, data.y, data.z);
  graphene_vec3_init (&event->delta, data.deltaX, data.deltaY, data.deltaZ);
  event->time = data.fUpdateTime;

  g_signal_emit (self, action_signals[ANALOG_EVENT], 0, event);

  return TRUE;
}

gboolean
openvr_action_poll_pose (OpenVRAction *self)
{
  OpenVRContext *context = openvr_context_get_instance ();

  InputPoseActionData_t data;

  float predicted_seconds_from_now = 0;

  EVRInputError err;
  err = context->input->GetPoseActionData (self->handle,
                                           context->origin,
                                           predicted_seconds_from_now,
                                           &data,
                                           sizeof(data),
                                           k_ulInvalidInputValueHandle);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: GetPoseActionData: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }

  OpenVRPoseEvent *event = g_malloc (sizeof (OpenVRPoseEvent));
  event->active = data.bActive;
  openvr_math_matrix34_to_graphene (&data.pose.mDeviceToAbsoluteTracking,
                                    &event->pose);
  graphene_vec3_init_from_float (&event->velocity,
                                 data.pose.vVelocity.v);
  graphene_vec3_init_from_float (&event->angular_velocity,
                                 data.pose.vAngularVelocity.v);
  event->valid = data.pose.bPoseIsValid;
  event->device_connected = data.pose.bDeviceIsConnected;

  g_signal_emit (self, action_signals[POSE_EVENT], 0, event);

  return TRUE;
}

gboolean
openvr_action_trigger_haptic (OpenVRAction *self,
                              float start_seconds_from_now,
                              float duration_seconds,
                              float frequency,
                              float amplitude)
{
  OpenVRContext *context = openvr_context_get_instance ();

  EVRInputError err;
  err = context->input->TriggerHapticVibrationAction (
    self->handle,
    start_seconds_from_now,
    duration_seconds,
    frequency,
    amplitude,
    k_ulInvalidInputValueHandle);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: TriggerHapticVibrationAction: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }

  return TRUE;
}

static void
openvr_action_finalize (GObject *gobject)
{
  OpenVRAction *self = OPENVR_ACTION (gobject);
  (void) self;
}

OpenVRActionType
openvr_action_get_action_type (OpenVRAction *self)
{
  return self->type;
}
