/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-system.h"
#include <openvr.h>

G_DEFINE_TYPE (OpenVRSystem, openvr_system, G_TYPE_OBJECT)

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

static gboolean
_vr_init (OpenVRSystem * self, vr::EVRApplicationType app_type)
{
  vr::HmdError error;
  self->system = vr::VR_Init (&error, app_type);

  if (error != vr::VRInitError_None) {
    g_print ("Could not init OpenVR runtime: Error code %d\n", error);
    return FALSE;
  }

  return TRUE;
}

gboolean
openvr_system_init_overlay (OpenVRSystem * self)
{
  return _vr_init (self, vr::VRApplication_Overlay);
}

#define STRING_BUFFER_SIZE 128

static gchar*
_get_device_string (OpenVRSystem * self,
                    vr::TrackedDeviceIndex_t device_index,
                    vr::TrackedDeviceProperty property)
{
  gchar * string = static_cast<gchar*> (g_malloc (STRING_BUFFER_SIZE));

  vr::TrackedPropertyError err;
  self->system->GetStringTrackedDeviceProperty (
    device_index, property, string, STRING_BUFFER_SIZE, &err);

  if (err != vr::TrackedProp_Success)
    g_print("Error getting string: %s\n",
      self->system->GetPropErrorNameFromEnum (err));

  return string;
}

void
openvr_system_print_device_info (OpenVRSystem * self)
{
  gchar* tracking_system_name =
    _get_device_string (self, vr::k_unTrackedDeviceIndex_Hmd,
                        vr::Prop_TrackingSystemName_String);
  g_print ("TrackingSystemName: %s\n", tracking_system_name);
  g_free (tracking_system_name);

  gchar* serial_number =
    _get_device_string (self, vr::k_unTrackedDeviceIndex_Hmd,
                        vr::Prop_SerialNumber_String);
  g_print ("SerialNumber: %s\n", serial_number);
  g_free (serial_number);
}

gboolean
openvr_system_is_available (void)
{
  return vr::VRSystem() != nullptr;
}

gboolean
openvr_system_is_compositor_available (void)
{
  return vr::VRCompositor() != nullptr;
}

void
openvr_system_shutdown (void)
{
  vr::VR_Shutdown();
}
