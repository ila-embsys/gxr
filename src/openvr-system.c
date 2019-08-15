/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib/gprintf.h>

#include "openvr-wrapper.h"

#include "openvr-context.h"
#include "openvr-system.h"
#include "gxr-math.h"
#include "openvr-context-private.h"

#define STRING_BUFFER_SIZE 128

gchar*
openvr_system_get_device_string (TrackedDeviceIndex_t device_index,
                                 ETrackedDeviceProperty property)
{
  gchar *string = (gchar*) g_malloc (STRING_BUFFER_SIZE);

  OpenVRContext *context = openvr_context_get_instance ();
  OpenVRFunctions *f = openvr_context_get_functions (context);

  ETrackedPropertyError error;
  f->system->GetStringTrackedDeviceProperty(
    device_index, property, string, STRING_BUFFER_SIZE, &error);

  if (error != ETrackedPropertyError_TrackedProp_Success)
    g_print ("Error getting string: %s\n",
      f->system->GetPropErrorNameFromEnum (error));

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
  OpenVRFunctions *f = openvr_context_get_functions (context);

  HmdMatrix44_t openvr_mat =
    f->system->GetProjectionMatrix (eye, near, far);

  graphene_matrix_t mat;
  gxr_math_matrix44_to_graphene (&openvr_mat, &mat);
  return mat;
}

graphene_matrix_t
openvr_system_get_eye_to_head_transform (EVREye eye)
{
  OpenVRContext *context = openvr_context_get_instance ();
  OpenVRFunctions *f = openvr_context_get_functions (context);

  HmdMatrix34_t openvr_mat = f->system->GetEyeToHeadTransform (eye);

  graphene_matrix_t mat;
  gxr_math_matrix34_to_graphene (&openvr_mat, &mat);
  return mat;
}

gboolean
openvr_system_get_hmd_pose (graphene_matrix_t *pose)
{
  OpenVRContext *context = openvr_context_get_instance ();
  OpenVRFunctions *f = openvr_context_get_functions (context);

  VRControllerState_t state;
  if (f->system->IsTrackedDeviceConnected(k_unTrackedDeviceIndex_Hmd) &&
      f->system->GetTrackedDeviceClass (k_unTrackedDeviceIndex_Hmd) ==
          ETrackedDeviceClass_TrackedDeviceClass_HMD &&
      f->system->GetControllerState (k_unTrackedDeviceIndex_Hmd,
                                           &state, sizeof(state)))
    {
      /* k_unTrackedDeviceIndex_Hmd should be 0 => posearray[0] */
      ETrackingUniverseOrigin origin = openvr_context_get_origin (context);

      TrackedDevicePose_t openvr_pose;
      f->system->GetDeviceToAbsoluteTrackingPose (origin, 0, &openvr_pose, 1);
      gxr_math_matrix34_to_graphene (&openvr_pose.mDeviceToAbsoluteTracking,
                                     pose);

      return openvr_pose.bDeviceIsConnected &&
             openvr_pose.bPoseIsValid &&
             openvr_pose.eTrackingResult ==
                 ETrackingResult_TrackingResult_Running_OK;
    }
  return FALSE;
}

gboolean
openvr_system_is_input_available (void)
{
  OpenVRContext *context = openvr_context_get_instance ();
  OpenVRFunctions *f = openvr_context_get_functions (context);
  return f->system->IsInputAvailable ();
}

gboolean
openvr_system_is_tracked_device_connected (uint32_t i)
{
  OpenVRContext *context = openvr_context_get_instance ();
  OpenVRFunctions *f = openvr_context_get_functions (context);
  return f->system->IsTrackedDeviceConnected (i);
}

gboolean
openvr_system_device_is_controller (uint32_t i)
{
  OpenVRContext *context = openvr_context_get_instance ();
  OpenVRFunctions *f = openvr_context_get_functions (context);
  return f->system->GetTrackedDeviceClass (i) ==
    ETrackedDeviceClass_TrackedDeviceClass_Controller;
}

void
openvr_system_get_render_target_size (uint32_t *w, uint32_t *h)
{
  OpenVRContext *context = openvr_context_get_instance ();
  OpenVRFunctions *f = openvr_context_get_functions (context);
  f->system->GetRecommendedRenderTargetSize (w, h);
}

/**
 * openvr_system_get_frustum_angles:
 * @left: The angle from the center view axis to the left in deg.
 * @right: The angle from the center view axis to the right in deg.
 * @top: The angle from the center view axis to the top in deg.
 * @bottom: The angle from the center view axis to the bottom in deg.
 *
 * Left and bottom are usually negative, top and right usually positive.
 * Exceptions are FOVs where both opposing sides are on the same side of the
 * center view axis (e.g. 2+ viewports per eye).
 */

#define PI 3.1415926535f
#define RAD_TO_DEG(x) ( (x) * 360.0f / ( 2.0f * PI ) )

void
openvr_system_get_frustum_angles (float *left, float *right,
                                  float *top, float *bottom)
{
  OpenVRContext *context = openvr_context_get_instance ();
  OpenVRFunctions *f = openvr_context_get_functions (context);
  f->system->GetProjectionRaw (EVREye_Eye_Left, left, right, top, bottom);

  *left = RAD_TO_DEG (atanf (*left));
  *right = RAD_TO_DEG (atanf (*right));
  *top = - RAD_TO_DEG (atanf (*top));
  *bottom = - RAD_TO_DEG (atanf (*bottom));
}
