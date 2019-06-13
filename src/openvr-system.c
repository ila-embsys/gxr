/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib/gprintf.h>

#include "openvr-wrapper.h"

#include "openvr-context.h"
#include "openvr-system.h"
#include "openvr-math.h"

#define STRING_BUFFER_SIZE 128

gchar*
openvr_system_get_device_string (TrackedDeviceIndex_t device_index,
                                 ETrackedDeviceProperty property)
{
  gchar *string = (gchar*) g_malloc (STRING_BUFFER_SIZE);

  OpenVRContext *context = openvr_context_get_instance ();

  ETrackedPropertyError error;
  context->system->GetStringTrackedDeviceProperty(
    device_index, property, string, STRING_BUFFER_SIZE, &error);

  if (error != ETrackedPropertyError_TrackedProp_Success)
    g_print ("Error getting string: %s\n",
      context->system->GetPropErrorNameFromEnum (error));

  return string;
}

void
openvr_system_print_device_info ()
{
  gchar* tracking_system_name =
    openvr_system_get_device_string (k_unTrackedDeviceIndex_Hmd,
      ETrackedDeviceProperty_Prop_TrackingSystemName_String);
  g_print ("TrackingSystemName: %s\n", tracking_system_name);
  g_free (tracking_system_name);

  gchar* serial_number =
    openvr_system_get_device_string (k_unTrackedDeviceIndex_Hmd,
      ETrackedDeviceProperty_Prop_SerialNumber_String);
  g_print ("SerialNumber: %s\n", serial_number);
  g_free (serial_number);
}

graphene_matrix_t
openvr_system_get_projection_matrix (EVREye eye, float near, float far)
{
  OpenVRContext *context = openvr_context_get_instance ();
  HmdMatrix44_t openvr_mat =
    context->system->GetProjectionMatrix (eye, near, far);

  graphene_matrix_t mat;
  openvr_math_matrix44_to_graphene (&openvr_mat, &mat);
  return mat;
}

graphene_matrix_t
openvr_system_get_eye_to_head_transform (EVREye eye)
{
  OpenVRContext *context = openvr_context_get_instance ();
  HmdMatrix34_t openvr_mat = context->system->GetEyeToHeadTransform (eye);

  graphene_matrix_t mat;
  openvr_math_matrix34_to_graphene (&openvr_mat, &mat);
  return mat;
}

gboolean
openvr_system_get_hmd_pose (graphene_matrix_t *pose)
{
  OpenVRContext *context = openvr_context_get_instance ();
  VRControllerState_t state;
  if (context->system->IsTrackedDeviceConnected(k_unTrackedDeviceIndex_Hmd) &&
      context->system->GetTrackedDeviceClass (k_unTrackedDeviceIndex_Hmd) ==
          ETrackedDeviceClass_TrackedDeviceClass_HMD &&
      context->system->GetControllerState (k_unTrackedDeviceIndex_Hmd,
                                           &state, sizeof(state)))
    {
      /* k_unTrackedDeviceIndex_Hmd should be 0 => posearray[0] */
      TrackedDevicePose_t openvr_pose;
      context->system->GetDeviceToAbsoluteTrackingPose (context->origin, 0,
                                                        &openvr_pose, 1);
      openvr_math_matrix34_to_graphene (&openvr_pose.mDeviceToAbsoluteTracking,
                                        pose);

      return openvr_pose.bDeviceIsConnected &&
             openvr_pose.bPoseIsValid &&
             openvr_pose.eTrackingResult ==
                 ETrackingResult_TrackingResult_Running_OK;
    }
  return FALSE;
}
