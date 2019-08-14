/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gdk/gdk.h>

#include "openvr-wrapper.h"
#include "gxr-math.h"
#include "openvr-action.h"
#include "openvr-context.h"
#include "openvr-action-set.h"

struct _OpenVRAction
{
  GObject parent;

  VRActionHandle_t handle;

  OpenVRActionSet *action_set;
  gchar *url;

  GSList *input_handles;

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
  self->handle = k_ulInvalidActionHandle;
  self->input_handles = NULL;
  self->action_set = NULL;
  self->url = NULL;
}

static gboolean
_input_handle_already_known (OpenVRAction *self, VRInputValueHandle_t handle)
{
  for(GSList *e = self->input_handles; e; e = e->next)
    {
      VRActionHandle_t *input_handle = e->data;
      if (handle == *input_handle)
        return TRUE;
    }
  return FALSE;
}

void
openvr_action_update_input_handles (OpenVRAction *self)
{
  OpenVRContext *context = openvr_context_get_instance ();

  VRActionSetHandle_t set_handle =
    openvr_action_set_get_handle (self->action_set);

  VRInputValueHandle_t origin_handles[k_unMaxActionOriginCount];
  EVRInputError err =
    context->input->GetActionOrigins (set_handle, self->handle,
                                      origin_handles, k_unMaxActionOriginCount);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("GetActionOrigins for %s failed, retrying later...\n",
                  self->url);
      return;
    }

  int origin_count = -1;
  while (origin_handles[++origin_count] != k_ulInvalidInputValueHandle);

  /* g_print ("Action %s has %d origins\n", self->url, origin_count); */

  for (int i = 0; i < origin_count; i++)
    {
      InputOriginInfo_t origin_info;
      err = context->input->GetOriginTrackedDeviceInfo (origin_handles[i],
                                                        &origin_info,
                                                        sizeof (origin_info));
      if (err != EVRInputError_VRInputError_None)
        {
          g_printerr ("GetOriginTrackedDeviceInfo for %s failed\n", self->url);
          return;
        }

      if (_input_handle_already_known (self, origin_info.devicePath))
        continue;

      VRInputValueHandle_t *input_handle =
        g_malloc (sizeof (VRInputValueHandle_t));
      *input_handle = origin_info.devicePath;
      self->input_handles = g_slist_append (self->input_handles, input_handle);

      /* TODO: origin localized name max length same as action name? */
      char origin_name[k_unMaxActionNameLength];
      context->input->GetOriginLocalizedName (origin_handles[i], origin_name,
                                              k_unMaxActionNameLength,
                                              EVRInputStringBits_VRInputString_All);
      g_print ("Added origin %s for action %s\n", origin_name, self->url);
    }
}

OpenVRAction *
openvr_action_new (void)
{
  return (OpenVRAction*) g_object_new (OPENVR_TYPE_ACTION, 0);
}

OpenVRAction *
openvr_action_new_from_url (OpenVRActionSet *action_set, char *url)
{
  OpenVRAction *self = openvr_action_new ();
  if (!openvr_action_load_handle (self, url))
    {
      g_object_unref (self);
      self = NULL;
    }

  self->url = g_strdup (url);
  self->action_set = action_set;
  return self;
}

OpenVRAction *
openvr_action_new_from_type_url (OpenVRActionSet *action_set,
                                 OpenVRActionType type, char *url)
{
  OpenVRAction *self = openvr_action_new ();
  self->type = type;
  if (!openvr_action_load_handle (self, url))
    {
      g_object_unref (self);
      self = NULL;
    }

  self->url = g_strdup (url);
  self->action_set = action_set;
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
  for(GSList *e = self->input_handles; e; e = e->next)
    {
      VRActionHandle_t *input_handle = e->data;

      err = context->input->GetDigitalActionData (self->handle, &data,
                                                  sizeof(data),
                                                  *input_handle);

      if (err != EVRInputError_VRInputError_None)
        {
          g_printerr ("ERROR: GetDigitalActionData: %s\n",
                      openvr_input_error_string (err));
          return FALSE;
        }

      InputOriginInfo_t origin_info;
      err = context->input->GetOriginTrackedDeviceInfo (data.activeOrigin,
                                                        &origin_info,
                                                        sizeof (origin_info));

      if (err != EVRInputError_VRInputError_None)
        {
          /* controller is not active, but might be active later */
          if (err == EVRInputError_VRInputError_InvalidHandle)
            continue;

          g_printerr ("ERROR: GetOriginTrackedDeviceInfo: %s\n",
                      openvr_input_error_string (err));
          return FALSE;
        }

      OpenVRDigitalEvent *event = g_malloc (sizeof (OpenVRDigitalEvent));
      event->controller_handle = origin_info.trackedDeviceIndex;
      event->active = data.bActive;
      event->state = data.bState;
      event->changed = data.bChanged;
      event->time = data.fUpdateTime;

      g_signal_emit (self, action_signals[DIGITAL_EVENT], 0, event);
    }
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
      return openvr_action_poll_pose_secs_from_now (self, 0);
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

  for(GSList *e = self->input_handles; e; e = e->next)
    {
      VRActionHandle_t *input_handle = e->data;
      err = context->input->GetAnalogActionData (self->handle, &data,
                                                 sizeof(data),
                                                 *input_handle);

      if (err != EVRInputError_VRInputError_None)
        {
          g_printerr ("ERROR: GetAnalogActionData: %s\n",
                      openvr_input_error_string (err));
          return FALSE;
        }

      InputOriginInfo_t origin_info;
      err = context->input->GetOriginTrackedDeviceInfo (data.activeOrigin,
                                                        &origin_info,
                                                        sizeof (origin_info));

      if (err != EVRInputError_VRInputError_None)
        {
          /* controller is not active, but might be active later */
          if (err == EVRInputError_VRInputError_InvalidHandle)
            continue;

          g_printerr ("ERROR: GetOriginTrackedDeviceInfo: %s\n",
                      openvr_input_error_string (err));
          return FALSE;
        }

      OpenVRAnalogEvent *event = g_malloc (sizeof (OpenVRAnalogEvent));
      event->active = data.bActive;
      event->controller_handle = origin_info.trackedDeviceIndex;
      graphene_vec3_init (&event->state, data.x, data.y, data.z);
      graphene_vec3_init (&event->delta, data.deltaX, data.deltaY, data.deltaZ);
      event->time = data.fUpdateTime;

      g_signal_emit (self, action_signals[ANALOG_EVENT], 0, event);
    }

  return TRUE;
}

static gboolean
_emit_pose_event (OpenVRAction          *self,
                  InputPoseActionData_t *data)
{
  OpenVRContext *context = openvr_context_get_instance ();

  InputOriginInfo_t origin_info;
  EVRInputError err;
  err = context->input->GetOriginTrackedDeviceInfo (data->activeOrigin,
                                                   &origin_info,
                                                    sizeof (origin_info));
  if (err != EVRInputError_VRInputError_None)
    {
      /* controller is not active, but might be active later */
      if (err == EVRInputError_VRInputError_InvalidHandle)
        return TRUE;

      g_printerr ("ERROR: GetOriginTrackedDeviceInfo: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }

  OpenVRPoseEvent *event = g_malloc (sizeof (OpenVRPoseEvent));
  event->active = data->bActive;
  event->controller_handle = origin_info.trackedDeviceIndex;
  gxr_math_matrix34_to_graphene (&data->pose.mDeviceToAbsoluteTracking,
                                    &event->pose);
  graphene_vec3_init_from_float (&event->velocity,
                                 data->pose.vVelocity.v);
  graphene_vec3_init_from_float (&event->angular_velocity,
                                 data->pose.vAngularVelocity.v);
  event->valid = data->pose.bPoseIsValid;
  event->device_connected = data->pose.bDeviceIsConnected;

  g_signal_emit (self, action_signals[POSE_EVENT], 0, event);

  return TRUE;
}

gboolean
openvr_action_poll_pose_secs_from_now (OpenVRAction *self,
                                       float         secs)
{
  OpenVRContext *context = openvr_context_get_instance ();
  EVRInputError err;

  for(GSList *e = self->input_handles; e; e = e->next)
    {
      VRActionHandle_t *input_handle = e->data;
      InputPoseActionData_t data;
      err = context->input->GetPoseActionDataRelativeToNow (self->handle,
                                                            context->origin,
                                                            secs,
                                                           &data,
                                                            sizeof(data),
                                                           *input_handle);
      if (err != EVRInputError_VRInputError_None)
        {
          g_printerr ("ERROR: GetPoseActionData: %s\n",
                      openvr_input_error_string (err));
          return FALSE;
        }

      if (!_emit_pose_event (self, &data))
        return FALSE;

    }
  return TRUE;
}

gboolean
openvr_action_poll_pose_next_frame (OpenVRAction *self)
{
  OpenVRContext *context = openvr_context_get_instance ();
  EVRInputError err;

  for(GSList *e = self->input_handles; e; e = e->next)
    {
      VRActionHandle_t *input_handle = e->data;
      InputPoseActionData_t data;
      err = context->input->GetPoseActionDataForNextFrame (self->handle,
                                                           context->origin,
                                                          &data,
                                                           sizeof(data),
                                                          *input_handle);
      if (err != EVRInputError_VRInputError_None)
        {
          g_printerr ("ERROR: GetPoseActionData: %s\n",
                      openvr_input_error_string (err));
          return FALSE;
        }

      if (!_emit_pose_event (self, &data))
        return FALSE;

    }
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
  g_slist_free_full (self->input_handles, g_free);
  g_free (self->url);
}

OpenVRActionType
openvr_action_get_action_type (OpenVRAction *self)
{
  return self->type;
}
