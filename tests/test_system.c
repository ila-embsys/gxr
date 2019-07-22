/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include "gxr.h"

static void
_test_minimal ()
{
  OpenVRContext *context = openvr_context_get_instance ();
  g_assert_nonnull (context);
  g_assert (openvr_context_initialize (context, OPENVR_APP_OVERLAY));
  g_assert (openvr_context_is_valid (context));

  openvr_system_print_device_info ();
  g_object_unref (context);
}

int
main ()
{
  _test_minimal ();

  return 0;
}
