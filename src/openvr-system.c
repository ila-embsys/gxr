/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib/gprintf.h>
#include <openvr_capi.h>
#include "openvr_capi_global.h"

#include "openvr-context.h"
#include "openvr-system.h"

G_DEFINE_TYPE (OpenVRSystem, openvr_system, G_TYPE_OBJECT)

static void
openvr_system_init (OpenVRSystem *self)
{
}

static void
openvr_system_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (openvr_system_parent_class)->finalize (gobject);
}

static void
openvr_system_class_init (OpenVRSystemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = openvr_system_finalize;
}

OpenVRSystem *
openvr_system_new (void)
{
  return (OpenVRSystem*) g_object_new (OPENVR_TYPE_SYSTEM, 0);
}

#define STRING_BUFFER_SIZE 128

static gchar*
_get_device_string (OpenVRSystem * self,
                    TrackedDeviceIndex_t device_index,
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
