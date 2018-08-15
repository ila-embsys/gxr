/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

void
_test_minimal ()
{
  OpenVRVulkanRenderer *renderer = openvr_vulkan_renderer_new ();
  g_assert_nonnull (renderer);

  g_assert (openvr_vulkan_client_init_vulkan (OPENVR_VULKAN_CLIENT (renderer),
                                              NULL, NULL, true));
  g_object_unref (renderer);
}

void
_test_glfw ()
{
  glfwInit ();

  glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint (GLFW_RESIZABLE, false);

  GLFWwindow *window = glfwCreateWindow (100, 100, "Test", NULL, NULL);

  glfwDestroyWindow (window);
  glfwTerminate ();
}

void
_test_glfw_extensions ()
{
  glfwInit ();

  uint32_t num_glfw_extensions = 0;
  const char** glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions (&num_glfw_extensions);

  for (int i = 0; i < num_glfw_extensions; i++)
    {
      g_print ("GLFW wants %s\n", glfw_extensions[i]);
    }

  glfwTerminate ();
}

void
_test_extensions_surface ()
{
  glfwInit ();

  glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint (GLFW_RESIZABLE, false);

  GLFWwindow *window = glfwCreateWindow (100, 100, "Test", NULL, NULL);

  OpenVRVulkanRenderer *renderer = openvr_vulkan_renderer_new ();
  g_assert_nonnull (renderer);

  uint32_t num_glfw_extensions = 0;
  const char** glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions (&num_glfw_extensions);

  GSList *instance_ext_list = NULL;
  for (int i = 0; i < num_glfw_extensions; i++)
    {
      instance_ext_list = g_slist_append (instance_ext_list,
                                          (char *) glfw_extensions[i]);
    }

  const gchar *device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

  GSList *device_ext_list = NULL;
  for (int i = 0; i < G_N_ELEMENTS (device_extensions); i++)
    {
      device_ext_list = g_slist_append (device_ext_list,
                                        (char *) device_extensions[i]);
    }

  OpenVRVulkanClient *client = OPENVR_VULKAN_CLIENT (renderer);
  g_assert (openvr_vulkan_client_init_vulkan (client,
                                              instance_ext_list,
                                              device_ext_list,
                                              true));

  VkSurfaceKHR surface;
  VkResult res = glfwCreateWindowSurface (client->instance->instance,
                                          window, NULL, &surface);
  g_assert (res == VK_SUCCESS);

  vkDestroySurfaceKHR (client->instance->instance, surface, NULL);

  g_object_unref (renderer);

  glfwDestroyWindow (window);
  glfwTerminate ();

  g_slist_free (instance_ext_list);
  g_slist_free (device_ext_list);
}

int
main (int argc, char *argv[])
{
  _test_minimal ();
  _test_glfw ();
  _test_glfw_extensions ();
  _test_extensions_surface ();

  return 0;
}


