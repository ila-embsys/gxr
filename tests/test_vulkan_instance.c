/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-instance.h"

void
_test_minimal ()
{
  OpenVRVulkanInstance *instance = openvr_vulkan_instance_new ();

  g_assert (openvr_vulkan_instance_create (instance, false, NULL));
  g_assert_nonnull (instance);

  g_object_unref (instance);
}

void
_test_validation ()
{
  OpenVRVulkanInstance *instance = openvr_vulkan_instance_new ();

  g_assert (openvr_vulkan_instance_create (instance, true, NULL));
  g_assert_nonnull (instance);

  g_object_unref (instance);
}

void
_test_extensions ()
{
  OpenVRVulkanInstance *instance = openvr_vulkan_instance_new ();

  GSList *instance_extensions = NULL;
  instance_extensions = g_slist_append (instance_extensions, "VK_KHR_surface");
  g_assert (openvr_vulkan_instance_create (instance,
                                           true,
                                           instance_extensions));
  g_assert_nonnull (instance);

  g_slist_free (instance_extensions);
  g_object_unref (instance);
}

int
main(int argc, char *argv[])
{
  _test_minimal ();
  _test_validation ();
  _test_extensions ();

  return 0;
}


