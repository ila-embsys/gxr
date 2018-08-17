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
#include "openvr-context.h"

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
  OpenVRContext *context = openvr_context_get_instance ();
  g_assert_nonnull (context);
  g_assert (openvr_context_init_overlay (context));
  g_assert (openvr_context_is_valid (context));
  g_object_unref (context);
}

int
main (int argc, char *argv[])
{
  _test_vr_init_shutdown ();
  _test_minimal ();

  return 0;
}
