/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib/gprintf.h>
#include <openvr_capi.h>
#include "openvr_capi_global.h"

#include "openvr-system.h"

#include "openvr-global.h"

G_DEFINE_TYPE (OpenVRSystem, openvr_system, G_TYPE_OBJECT)

static void
openvr_system_class_init (OpenVRSystemClass *klass)
{}

static void
openvr_system_init (OpenVRSystem *self)
{
  self->functions = NULL;
}

gboolean
_system_init_fn_table (OpenVRSystem *self)
{
  INIT_FN_TABLE (self->functions, System);
}

OpenVRSystem *
openvr_system_new (void)
{
  return (OpenVRSystem*) g_object_new (OPENVR_TYPE_SYSTEM, 0);
}

static gboolean
_vr_init (OpenVRSystem * self, EVRApplicationType app_type)
{
  EVRInitError error;
  VR_InitInternal(&error, app_type);

  if (error != EVRInitError_VRInitError_None) {
    g_printerr ("Could not init OpenVR runtime: %s: %s\n",
                VR_GetVRInitErrorAsSymbol (error),
                VR_GetVRInitErrorAsEnglishDescription (error));
    return FALSE;
  }

  _system_init_fn_table (self);

  return TRUE;
}

gboolean
openvr_system_init_overlay (OpenVRSystem * self)
{
  return _vr_init (self, EVRApplicationType_VRApplication_Overlay);
}

#define STRING_BUFFER_SIZE 128

static gchar*
_get_device_string (OpenVRSystem * self,
                    TrackedDeviceIndex_t device_index,
                    ETrackedDeviceProperty property)
{
  gchar *string = (gchar*) g_malloc (STRING_BUFFER_SIZE);

  ETrackedPropertyError error;
  self->functions->GetStringTrackedDeviceProperty(
    device_index, property, string, STRING_BUFFER_SIZE, &error);

  if (error != ETrackedPropertyError_TrackedProp_Success)
    g_print ("Error getting string: %s\n",
      self->functions->GetPropErrorNameFromEnum (error));

  return string;
}

void
openvr_system_print_device_info (OpenVRSystem * self)
{
  gchar* tracking_system_name =
    _get_device_string (self, k_unTrackedDeviceIndex_Hmd,
                        ETrackedDeviceProperty_Prop_TrackingSystemName_String);
  g_print ("TrackingSystemName: %s\n", tracking_system_name);
  g_free (tracking_system_name);

  gchar* serial_number =
    _get_device_string (self, k_unTrackedDeviceIndex_Hmd,
                        ETrackedDeviceProperty_Prop_SerialNumber_String);
  g_print ("SerialNumber: %s\n", serial_number);
  g_free (serial_number);
}

gboolean
openvr_system_is_available (OpenVRSystem * self)
{
  return self->functions != NULL;
}

gboolean
openvr_system_is_installed (void)
{
  return VR_IsRuntimeInstalled ();
}

gboolean
openvr_system_is_hmd_present (void)
{
  return VR_IsHmdPresent ();
}

void
openvr_system_shutdown (void)
{
  VR_ShutdownInternal();
}
