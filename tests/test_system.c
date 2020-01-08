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
  OpenVRContext *context = OPENVR_CONTEXT (gxr_context_get_instance ());
  g_assert_nonnull (context);
  g_assert (gxr_context_init_runtime (GXR_CONTEXT (context), GXR_APP_OVERLAY));
  g_assert (gxr_context_is_valid (GXR_CONTEXT (context)));
  g_object_unref (context);
}

int
main ()
{
  _test_minimal ();

  return 0;
}
