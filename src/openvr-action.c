/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gdk/gdk.h>

#include "gxr-types.h"

#include "openvr-wrapper.h"
#include "openvr-math-private.h"
#include "openvr-action.h"
#include "openvr-context.h"
#include "openvr-action-set.h"
#include "openvr-action-set-private.h"
#include "openvr-context-private.h"

struct _OpenVRAction
{
  GxrAction parent;

  VRActionHandle_t handle;
  GSList *input_handles;
};

G_DEFINE_TYPE (OpenVRAction, openvr_action, GXR_TYPE_ACTION)

gboolean
openvr_action_load_manifest (char *path)
{
  OpenVRFunctions *f = openvr_get_functions ();

  EVRInputError err;
  err = f->input->SetActionManifestPath (path);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: SetActionManifestPath: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }
  return TRUE;
}

gboolean
openvr_action_load_handle (OpenVRAction *self,
                           char         *url);

static void
openvr_action_init (OpenVRAction *self)
{
  self->handle = k_ulInvalidActionHandle;
  self->input_handles = NULL;
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
  OpenVRFunctions *f = openvr_get_functions ();

  GxrActionSet *action_set = gxr_action_get_action_set (GXR_ACTION (self));

  VRActionSetHandle_t actionset_handle =
    openvr_action_set_get_handle (OPENVR_ACTION_SET (action_set));

  VRInputValueHandle_t origin_handles[k_unMaxActionOriginCount];
  EVRInputError err =
    f->input->GetActionOrigins (actionset_handle, self->handle,
                                origin_handles, k_unMaxActionOriginCount);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("GetActionOrigins for %s failed, retrying later...\n",
                  gxr_action_get_url (GXR_ACTION (self)));
      return;
    }

  int origin_count = -1;
  while (origin_handles[++origin_count] != k_ulInvalidInputValueHandle);

  /* g_print ("Action %s has %d origins\n", self->url, origin_count); */

  for (int i = 0; i < origin_count; i++)
    {
      InputOriginInfo_t origin_info;
      err = f->input->GetOriginTrackedDeviceInfo (origin_handles[i],
                                                 &origin_info,
                                                  sizeof (origin_info));
      if (err != EVRInputError_VRInputError_None)
        {
          g_printerr ("GetOriginTrackedDeviceInfo for %s failed\n",
                      gxr_action_get_url (GXR_ACTION (self)));
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
      f->input->GetOriginLocalizedName (origin_handles[i], origin_name,
                                        k_unMaxActionNameLength,
                                        EVRInputStringBits_VRInputString_All);
      g_print ("Added origin %s for action %s\n", origin_name,
               gxr_action_get_url (GXR_ACTION (self)));
    }
}

OpenVRAction *
openvr_action_new (void)
{
  return (OpenVRAction*) g_object_new (OPENVR_TYPE_ACTION, 0);
}

OpenVRAction *
openvr_action_new_from_url (GxrActionSet *action_set, char *url)
{
  OpenVRAction *self = openvr_action_new ();
  gxr_action_set_url (GXR_ACTION (self), g_strdup (url));
  gxr_action_set_action_set(GXR_ACTION (self), action_set);

  if (!openvr_action_load_handle (self, url))
    {
      g_object_unref (self);
      self = NULL;
    }

  return self;
}

OpenVRAction *
openvr_action_new_from_type_url (GxrActionSet *action_set,
                                 GxrActionType type, char *url)
{
  OpenVRAction *self = openvr_action_new ();
  gxr_action_set_action_type (GXR_ACTION (self), type);
  gxr_action_set_url (GXR_ACTION (self), g_strdup (url));
  gxr_action_set_action_set(GXR_ACTION (self), action_set);

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
  OpenVRFunctions *f = openvr_get_functions ();
  EVRInputError err = f->input->GetActionHandle (url, &self->handle);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: GetActionHandle: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }
  return TRUE;
}

static gboolean
_action_poll_digital (OpenVRAction *self)
{
  OpenVRFunctions *f = openvr_get_functions ();

  InputDigitalActionData_t data;

  EVRInputError err;
  for(GSList *e = self->input_handles; e; e = e->next)
    {
      VRActionHandle_t *input_handle = e->data;

      err = f->input->GetDigitalActionData (self->handle, &data,
                                            sizeof(data), *input_handle);

      if (err != EVRInputError_VRInputError_None)
        {
          g_printerr ("ERROR: GetDigitalActionData: %s\n",
                      openvr_input_error_string (err));
          return FALSE;
        }

      InputOriginInfo_t origin_info;
      err = f->input->GetOriginTrackedDeviceInfo (data.activeOrigin,
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

      GxrDigitalEvent *event = g_malloc (sizeof (GxrDigitalEvent));
      event->controller_handle = origin_info.trackedDeviceIndex;
      event->active = data.bActive;
      event->state = data.bState;
      event->changed = data.bChanged;
      event->time = data.fUpdateTime;

      gxr_action_emit_digital (GXR_ACTION (self), event);
    }
  return TRUE;
}

static gboolean
_action_poll_analog (OpenVRAction *self)
{
  OpenVRFunctions *f = openvr_get_functions ();

  InputAnalogActionData_t data;

  EVRInputError err;

  for(GSList *e = self->input_handles; e; e = e->next)
    {
      VRActionHandle_t *input_handle = e->data;
      err = f->input->GetAnalogActionData (self->handle, &data,
                                           sizeof(data), *input_handle);

      if (err != EVRInputError_VRInputError_None)
        {
          g_printerr ("ERROR: GetAnalogActionData: %s\n",
                      openvr_input_error_string (err));
          return FALSE;
        }

      InputOriginInfo_t origin_info;
      err = f->input->GetOriginTrackedDeviceInfo (data.activeOrigin,
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

      GxrAnalogEvent *event = g_malloc (sizeof (GxrAnalogEvent));
      event->active = data.bActive;
      event->controller_handle = origin_info.trackedDeviceIndex;
      graphene_vec3_init (&event->state, data.x, data.y, data.z);
      graphene_vec3_init (&event->delta, data.deltaX, data.deltaY, data.deltaZ);
      event->time = data.fUpdateTime;

      gxr_action_emit_analog (GXR_ACTION (self), event);
    }

  return TRUE;
}

static gboolean
_emit_pose_event (OpenVRAction          *self,
                  InputPoseActionData_t *data)
{
  OpenVRFunctions *f = openvr_get_functions ();

  InputOriginInfo_t origin_info;
  EVRInputError err;
  err = f->input->GetOriginTrackedDeviceInfo (data->activeOrigin,
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

  GxrPoseEvent *event = g_malloc (sizeof (GxrPoseEvent));
  event->active = data->bActive;
  event->controller_handle = origin_info.trackedDeviceIndex;
  openvr_math_matrix34_to_graphene (&data->pose.mDeviceToAbsoluteTracking,
                                    &event->pose);
  graphene_vec3_init_from_float (&event->velocity,
                                 data->pose.vVelocity.v);
  graphene_vec3_init_from_float (&event->angular_velocity,
                                 data->pose.vAngularVelocity.v);
  event->valid = data->pose.bPoseIsValid;
  event->device_connected = data->pose.bDeviceIsConnected;

  gxr_action_emit_pose (GXR_ACTION (self), event);

  return TRUE;
}

static gboolean
_action_poll_pose_secs_from_now (OpenVRAction *self,
                                 float         secs)
{
  OpenVRFunctions *f = openvr_get_functions ();

  EVRInputError err;

  for(GSList *e = self->input_handles; e; e = e->next)
    {
      VRActionHandle_t *input_handle = e->data;

      InputPoseActionData_t data;

      OpenVRContext *context = OPENVR_CONTEXT (gxr_context_get_instance ());
      ETrackingUniverseOrigin origin = openvr_context_get_origin (context);

      err = f->input->GetPoseActionDataRelativeToNow (self->handle,
                                                      origin,
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

static gboolean
_poll (GxrAction *action)
{
  OpenVRAction *self = OPENVR_ACTION (action);
  GxrActionType type = gxr_action_get_action_type (action);
  switch (type)
    {
    case GXR_ACTION_DIGITAL:
      return _action_poll_digital (self);
    case GXR_ACTION_ANALOG:
      return _action_poll_analog (self);
    case GXR_ACTION_POSE:
      return _action_poll_pose_secs_from_now (self, 0);
    default:
      g_printerr ("Uknown action type %d\n", type);
      return FALSE;
    }
}

static gboolean
_trigger_haptic (GxrAction *action,
                 float start_seconds_from_now,
                 float duration_seconds,
                 float frequency,
                 float amplitude,
                 guint64 controller_handle)
{
  OpenVRAction *self = OPENVR_ACTION (action);

  OpenVRFunctions *f = openvr_get_functions ();

  EVRInputError err;
  err = f->input->TriggerHapticVibrationAction (
    self->handle,
    start_seconds_from_now,
    duration_seconds,
    frequency,
    amplitude,
    controller_handle);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: TriggerHapticVibrationAction: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }

  return TRUE;
}

static void
_finalize (GObject *gobject)
{
  OpenVRAction *self = OPENVR_ACTION (gobject);
  g_slist_free_full (self->input_handles, g_free);
}

static void
openvr_action_class_init (OpenVRActionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize =_finalize;

  GxrActionClass *gxr_action_class = GXR_ACTION_CLASS (klass);
  gxr_action_class->poll = _poll;
  gxr_action_class->trigger_haptic = _trigger_haptic;
}

