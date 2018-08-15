/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-uploader.h"

void
_test_minimal ()
{
  OpenVRVulkanUploader *uploader = openvr_vulkan_uploader_new ();
  g_assert_nonnull (uploader);

  OpenVRSystem * system = openvr_system_new ();
  g_assert (openvr_system_init_overlay (system));

  OpenVRCompositor *compositor = openvr_compositor_new ();
  g_assert (openvr_system_is_available (system));
  g_assert (openvr_system_is_installed ());

  g_assert (openvr_vulkan_uploader_init_vulkan (uploader, true,
                                                system, compositor));
  g_object_unref (uploader);
}

int
main (int argc, char *argv[])
{
  _test_minimal ();
  return 0;
}


