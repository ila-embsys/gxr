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

typedef struct Example
{
  OpenVRVulkanTexture *texture;
  GLFWwindow *window;
  GMainLoop *loop;
  VkSurfaceKHR surface;
  OpenVRVulkanRenderer *renderer;
  bool should_quit;
} Example;

GdkPixbuf *
load_gdk_pixbuf ()
{
  GError *error = NULL;
  GdkPixbuf * pixbuf_no_alpha =
    gdk_pixbuf_new_from_resource ("/res/cat.jpg", &error);

  if (error != NULL) {
    g_printerr ("Unable to read file: %s\n", error->message);
    g_error_free (error);
    return NULL;
  } else {
    GdkPixbuf *pixbuf = gdk_pixbuf_add_alpha (pixbuf_no_alpha, false, 0, 0, 0);
    g_object_unref (pixbuf_no_alpha);
    return pixbuf;
  }
}

void key_callback (GLFWwindow* window, int key,
                   int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
      Example *self = (Example*) glfwGetWindowUserPointer (window);
      self->should_quit = true;
    }
}

void
init_glfw (Example *self, int width, int height)
{
  glfwInit();

  glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint (GLFW_RESIZABLE, false);

  self->window = glfwCreateWindow (width, height, "Vulkan Pixbuf", NULL, NULL);

  glfwSetKeyCallback (self->window, key_callback);

  glfwSetWindowUserPointer (self->window, self);
}

gboolean
draw_cb (gpointer data)
{
  Example *self = (Example*) data;

  glfwPollEvents ();
  if (glfwWindowShouldClose (self->window) || self->should_quit)
    {
      g_main_loop_quit (self->loop);
      return FALSE;
    }

  openvr_vulkan_renderer_draw (self->renderer);

  vkDeviceWaitIdle (self->renderer->device->device);

  return TRUE;
}

int
main (int argc, char *argv[]) {
  Example example = {
    .should_quit = false
  };

  GdkPixbuf * pixbuf = load_gdk_pixbuf ();
  if (pixbuf == NULL)
    return -1;

  gint width = gdk_pixbuf_get_width (pixbuf);
  gint height = gdk_pixbuf_get_height (pixbuf);

  init_glfw (&example, width / 2, height / 2);

  example.loop = g_main_loop_new (NULL, FALSE);

  uint32_t num_glfw_extensions = 0;
  const char** glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions (&num_glfw_extensions);

  g_print ("GLFW requires %d instance extensions.\n", num_glfw_extensions);
  for (int i = 0; i < num_glfw_extensions; i++)
    {
      g_print ("%s\n", glfw_extensions[i]);
    }

  example.renderer = openvr_vulkan_renderer_new ();
  if (!openvr_vulkan_renderer_init_vulkan (example.renderer,
                                           example.surface,
                                           true))
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return -1;
  }

  if (glfwCreateWindowSurface (example.renderer->instance->instance,
                               example.window, NULL,
                              &example.surface) != VK_SUCCESS)
    {
      g_printerr ("Unable to create surface.\n");
      return -1;
    }

  example.texture = openvr_vulkan_texture_new ();

  openvr_vulkan_renderer_load_pixbuf (example.renderer,
                                      example.texture, pixbuf);

  if (!openvr_vulkan_renderer_init_rendering (example.renderer,
                                              example.surface,
                                              example.texture))
    return -1;

  g_timeout_add (20, draw_cb, &example);
  g_main_loop_run (example.loop);
  g_main_loop_unref (example.loop);

  g_object_unref (pixbuf);
  g_object_unref (example.texture);
  g_object_unref (example.renderer);

  glfwDestroyWindow (example.window);
  glfwTerminate ();

  return 0;
}
