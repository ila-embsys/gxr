/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-device.h"

void
_test_minimal ()
{
  OpenVRVulkanInstance *instance = openvr_vulkan_instance_new ();
  g_assert_nonnull (instance);

  g_assert (openvr_vulkan_instance_create (instance, true, NULL));

  OpenVRVulkanDevice *device = openvr_vulkan_device_new ();
  g_assert_nonnull (device);

  g_assert (openvr_vulkan_device_create (device, instance,
                                         VK_NULL_HANDLE, NULL));

  g_object_unref (device);
  g_object_unref (instance);
}

void
_test_extensions ()
{
  OpenVRVulkanInstance *instance = openvr_vulkan_instance_new ();
  g_assert_nonnull (instance);

  g_assert (openvr_vulkan_instance_create (instance, true, NULL));

  OpenVRVulkanDevice *device = openvr_vulkan_device_new ();
  g_assert_nonnull (device);

  GSList *extensions = NULL;
  extensions = g_slist_append (extensions, "VK_KHR_external_memory_fd");

  g_assert (openvr_vulkan_device_create (device, instance,
                                         VK_NULL_HANDLE, extensions));

  g_slist_free (extensions);

  g_object_unref (device);
  g_object_unref (instance);
}

int
main(int argc, char *argv[])
{
  _test_minimal ();
  _test_extensions ();

  return 0;
}


