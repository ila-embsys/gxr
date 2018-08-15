/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <openvr-glib.h>
#include <openvr_capi.h>
#include "openvr_capi_global.h"
#include "openvr-system.h"

void
_test_vr_init_shutdown ()
{
  EVRInitError error;
  VR_InitInternal (&error, EVRApplicationType_VRApplication_Overlay);
  g_assert (error == EVRInitError_VRInitError_None);
  VR_ShutdownInternal ();
}

void
_test_minimal ()
{
  OpenVRSystem * system = openvr_system_new ();

  g_assert (openvr_system_init_overlay (system));

  g_assert (openvr_system_is_available (system));
  g_assert (openvr_system_is_installed ());

  g_object_unref (system);
}

int
main (int argc, char *argv[])
{
  _test_vr_init_shutdown ();
  _test_minimal ();

  return 0;
}
