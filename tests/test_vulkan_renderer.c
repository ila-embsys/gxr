/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-renderer.h"

void
_test_minimal ()
{
  OpenVRVulkanRenderer *renderer = openvr_vulkan_renderer_new ();
  g_assert_nonnull (renderer);

  g_assert (openvr_vulkan_client_init_vulkan (OPENVR_VULKAN_CLIENT (renderer),
                                              NULL, NULL, true));
  g_object_unref (renderer);
}

int
main (int argc, char *argv[])
{
  _test_minimal ();

  return 0;
}


