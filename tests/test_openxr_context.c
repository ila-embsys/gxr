/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <gulkan.h>
#include "gxr.h"
#include "openxr-context.h"

static void
_test_minimal ()
{
  OpenXRContext *context = openxr_context_new ();
  g_assert_nonnull (context);

  GulkanClient *gc = gulkan_client_new ();
  g_assert (gulkan_client_init_vulkan (gc, NULL, NULL));

  GulkanInstance *gk_instance = gulkan_client_get_instance (gc);
  VkInstance vk_instance = gulkan_instance_get_handle (gk_instance);

  GulkanDevice *gk_device = gulkan_client_get_device (gc);
  VkDevice vk_device = gulkan_device_get_handle (gk_device);

  VkPhysicalDevice physical_device =
    gulkan_device_get_physical_handle (gk_device);

  uint32_t queue_family_index = gulkan_device_get_queue_family_index (gk_device);

  uint32_t queue_index = 0;

  g_assert (openxr_context_initialize (context, vk_instance,
                                       physical_device,
                                       vk_device,
                                       queue_family_index,
                                       queue_index));

  g_object_unref (context);
}

int
main ()
{
  _test_minimal ();

  return 0;
}
