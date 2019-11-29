/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */


#include <gxr.h>

static void
_test_minimal ()
{
  g_assert (openvr_context_is_installed ());
  OpenVRContext *context = OPENVR_CONTEXT (gxr_context_get_instance ());
  g_assert_nonnull (context);
  g_assert (openvr_context_initialize (context, GXR_APP_OVERLAY));
  g_assert (gxr_context_is_valid (GXR_CONTEXT (context)));

  GulkanClient *uploader = openvr_compositor_gulkan_client_new ();
  g_assert_nonnull (uploader);
  g_object_unref (uploader);
}

int
main ()
{
  _test_minimal ();
  return 0;
}


