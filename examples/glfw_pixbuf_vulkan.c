/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <openvr-glib.h>

#include "openvr-system.h"
#include "openvr-overlay.h"
#include "openvr-compositor.h"
#include "openvr-vulkan-renderer.h"

#define WIDTH 1280
#define HEIGHT 720

typedef struct Example
{
  OpenVRVulkanTexture *texture;
  GLFWwindow* window;
  GMainLoop *loop;
  VkSurfaceKHR surface;
} Example;

GdkPixbuf *
load_gdk_pixbuf ()
{
  GError *error = NULL;
  GdkPixbuf * pixbuf_unflipped =
    gdk_pixbuf_new_from_resource ("/res/cat.jpg", &error);

  if (error != NULL) {
    fprintf (stderr, "Unable to read file: %s\n", error->message);
    g_error_free (error);
    return NULL;
  } else {
    GdkPixbuf *pixbuf = gdk_pixbuf_add_alpha (pixbuf_unflipped, false, 0, 0, 0);
    g_object_unref (pixbuf_unflipped);
    return pixbuf;
  }
}

void
init_glfw (Example *self)
{
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  self->window = glfwCreateWindow(WIDTH, HEIGHT, "Pixbuf", NULL, NULL);
}

gboolean
draw_cb (gpointer data)
{
  Example *self = (Example*) data;

  glfwPollEvents();
  if (glfwWindowShouldClose(self->window))
    g_main_loop_quit (self->loop);

  return TRUE;
}

bool
_load_resource (const gchar* path, GBytes **res)
{
  GError *error = NULL;

  *res = g_resources_lookup_data ("/shaders/texture.frag.spv",
                                  G_RESOURCE_FLAGS_NONE,
                                 &error);

  if (error != NULL)
    {
      g_printerr ("Unable to read file: %s\n", error->message);
      g_error_free (error);
      return false;
    }

  return true;
}

VkShaderModule
_create_shader_module (VkDevice device, GBytes *shader)
{
  gsize shader_size = 0;
  const uint32_t *shader_buffer = g_bytes_get_data (shader, &shader_size);

  g_print ("Shader buffer size: %ld", shader_size);

  VkShaderModuleCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = shader_size,
    .pCode = shader_buffer,
  };

  VkShaderModule shader_module;
  if (vkCreateShaderModule (device, &info, NULL, &shader_module) != VK_SUCCESS)
    {
      g_printerr ("Failed to create shader module.\n");
    }

  return shader_module;
}

int
main (int argc, char *argv[]) {
  Example example = {};

  init_glfw (&example);

  GdkPixbuf * pixbuf = load_gdk_pixbuf ();

  if (pixbuf == NULL)
    return -1;

  example.loop = g_main_loop_new (NULL, FALSE);

  GBytes *fragment_shader;
  GBytes *vertex_shader;

  if (!_load_resource ("/shaders/texture.frag.spv", &fragment_shader))
    return -1;
  if (!_load_resource ("/shaders/texture.vert.spv", &vertex_shader))
    return -1;

  uint32_t num_glfw_extensions = 0;
  const char** glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions(&num_glfw_extensions);

  g_print ("GLFW requires %d instance extensions.\n", num_glfw_extensions);
  for (int i = 0; i < num_glfw_extensions; i++)
    {
      g_print ("%s\n", glfw_extensions[i]);
    }

  OpenVRVulkanRenderer *renderer = openvr_vulkan_renderer_new ();
  if (!openvr_vulkan_renderer_init_vulkan (renderer, example.surface, true))
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return -1;
  }

  if (glfwCreateWindowSurface(renderer->instance->instance,
                              example.window, NULL,
                              &example.surface) != VK_SUCCESS)
    {
      g_printerr ("Unable to create surface.\n");
      return -1;
    }

  g_print ("Device surface support: %d\n",
           openvr_vulkan_device_queue_supports_surface (renderer->device,
                                                        example.surface));

  if (!openvr_vulkan_renderer_init_swapchain (renderer->device->device,
                                              renderer->device->physical_device,
                                              example.surface))
    {
      g_printerr ("Failed to create swapchain.\n");
      return false;
    }

  VkShaderModule fragment_shader_module =
    _create_shader_module (renderer->device->device, fragment_shader);

  VkShaderModule vertex_shader_module =
    _create_shader_module (renderer->device->device, vertex_shader);

  vkDestroyShaderModule(renderer->device->device, fragment_shader_module, NULL);
  vkDestroyShaderModule(renderer->device->device, vertex_shader_module, NULL);

  g_bytes_unref (fragment_shader);
  g_bytes_unref (vertex_shader);

  example.texture = openvr_vulkan_texture_new ();

  openvr_vulkan_renderer_load_pixbuf (renderer, example.texture, pixbuf);

  g_timeout_add (20, draw_cb, &example);
  g_main_loop_run (example.loop);
  g_main_loop_unref (example.loop);

  g_object_unref (pixbuf);
  g_object_unref (example.texture);
  g_object_unref (renderer);

  glfwDestroyWindow(example.window);
  glfwTerminate();

  return 0;
}
