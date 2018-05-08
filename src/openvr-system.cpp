/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-system.h"
#include <openvr.h>

G_DEFINE_TYPE (OpenVRSystem, openvr_system, G_TYPE_OBJECT)

gchar*
openvr_system_get_device_string (vr::IVRSystem *system,
                                 vr::TrackedDeviceIndex_t device_index,
                                 vr::TrackedDeviceProperty property);

static void
openvr_system_class_init (OpenVRSystemClass *klass)
{
  g_print ("init system class\n");
}

static void
openvr_system_init (OpenVRSystem *self)
{
  g_print ("init system\n");
}

OpenVRSystem *
openvr_system_new (void)
{
  return static_cast<OpenVRSystem*> (g_object_new (OPENVR_TYPE_SYSTEM, 0));
}

gboolean
openvr_system_init_overlay (OpenVRSystem * self)
{
  vr::HmdError error;
  vr::IVRSystem *system = vr::VR_Init (&error, vr::VRApplication_Overlay);

  if (error != vr::VRInitError_None) {
    g_print("Could not init OpenVR runtime: Error code %d\n", error);
    return FALSE;
  }

  gchar* tracking_system_name =
    openvr_system_get_device_string (system, vr::k_unTrackedDeviceIndex_Hmd,
                                     vr::Prop_TrackingSystemName_String);
  g_print ("Prop_TrackingSystemName_String %s\n", tracking_system_name);
  g_free (tracking_system_name);

  gchar* serial_number =
    openvr_system_get_device_string (system, vr::k_unTrackedDeviceIndex_Hmd,
                                     vr::Prop_SerialNumber_String);
  g_print ("Prop_SerialNumber_String %s\n", serial_number);
  g_free (serial_number);

  return TRUE;
}

#define STRING_BUFFER_SIZE 128

gchar*
openvr_system_get_device_string (vr::IVRSystem *system,
                                 vr::TrackedDeviceIndex_t device_index,
                                 vr::TrackedDeviceProperty property)
{
  gchar * string = static_cast<gchar*> (g_malloc (STRING_BUFFER_SIZE));

  vr::TrackedPropertyError err;
  system->GetStringTrackedDeviceProperty (
    device_index, property, string, STRING_BUFFER_SIZE, &err);

  if (err != vr::TrackedProp_Success)
    g_print("Error getting string: %s\n",
      system->GetPropErrorNameFromEnum(err));

  return string;
}
