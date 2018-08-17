/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-client.h"

void
_test_minimal ()
{
  OpenVRVulkanClient *client = openvr_vulkan_client_new ();
  g_assert_nonnull (client);

  g_assert (openvr_vulkan_client_init_vulkan (client, NULL, NULL, true));

  g_object_unref (client);
}

int
main ()
{
  _test_minimal ();

  return 0;
}


