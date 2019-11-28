/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <gxr.h>
#include "openvr-wrapper.h"
#include "openvr-context.h"

static void
_test_vr_init_shutdown ()
{
  EVRInitError error;
  VR_InitInternal (&error, EVRApplicationType_VRApplication_Overlay);
  g_assert (error == EVRInitError_VRInitError_None);
  VR_ShutdownInternal ();
}

static void
_test_minimal ()
{
  OpenVRContext *context = OPENVR_CONTEXT (gxr_context_get_instance ());
  g_assert_nonnull (context);
  g_assert (openvr_context_initialize (context, GXR_APP_OVERLAY));
  g_assert (openvr_context_is_valid (context));
  g_object_unref (context);
}

int
main ()
{
  _test_vr_init_shutdown ();
  _test_minimal ();

  return 0;
}
