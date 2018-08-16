/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */


#include "openvr-context.h"
#include "openvr-vulkan-uploader.h"

void
_test_minimal ()
{
  OpenVRVulkanUploader *uploader = openvr_vulkan_uploader_new ();
  g_assert_nonnull (uploader);

  g_assert (openvr_context_is_installed ());
  OpenVRContext *context = openvr_context_get_instance ();
  g_assert_nonnull (context);
  g_assert (openvr_context_init_overlay (context));
  g_assert (openvr_context_is_valid (context));

  g_assert (openvr_vulkan_uploader_init_vulkan (uploader, true));
  g_object_unref (uploader);
}

int
main (int argc, char *argv[])
{
  _test_minimal ();
  return 0;
}


