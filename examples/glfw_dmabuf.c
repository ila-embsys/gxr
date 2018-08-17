/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

#include <amdgpu_drm.h>
#include <amdgpu.h>
#include <fcntl.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "openvr-vulkan-renderer.h"

#include "dmabuf_content.h"

typedef struct Example
{
  OpenVRVulkanTexture *texture;
  GLFWwindow *window;
  GMainLoop *loop;
  VkSurfaceKHR surface;
  OpenVRVulkanRenderer *renderer;
  bool should_quit;
  amdgpu_bo_handle amd_bo;
  amdgpu_device_handle amd_dev;
} Example;

void*
allocate_dmabuf_amd (Example *self, int size, int *fd)
{
  /* use render node to avoid needing to authenticate: */
  int dev_fd = open ("/dev/dri/renderD128", 02, 0);

  uint32_t major_version;
  uint32_t minor_version;
  int ret = amdgpu_device_initialize (dev_fd,
                                     &major_version,
                                     &minor_version,
                                     &self->amd_dev);
  if (ret < 0)
    {
      g_printerr ("Could not create amdgpu device: %s\n", strerror (-ret));
      return NULL;
    }

  g_print ("Initialized amdgpu drm device with fd %d. Version %d.%d\n",
           dev_fd, major_version, minor_version);

  struct amdgpu_bo_alloc_request alloc_buffer =
  {
    .alloc_size = (uint64_t) size,
    .preferred_heap = AMDGPU_GEM_DOMAIN_GTT,
  };

  ret = amdgpu_bo_alloc (self->amd_dev, &alloc_buffer, &self->amd_bo);
  if (ret < 0)
    {
      g_printerr ("amdgpu_bo_alloc failed: %s\n", strerror(-ret));
      return NULL;
    }

  uint32_t shared_handle;
  ret = amdgpu_bo_export (self->amd_bo,
                          amdgpu_bo_handle_type_dma_buf_fd,
                         &shared_handle);

  if (ret < 0)
    {
      g_printerr ("amdgpu_bo_export failed: %s\n", strerror (-ret));
      return NULL;
    }

  *fd = (int) shared_handle;
  void *cpu_buffer;
  ret = amdgpu_bo_cpu_map (self->amd_bo, &cpu_buffer);

  if (ret < 0)
    {
      g_printerr ("amdgpu_bo_cpu_map failed: %s\n", strerror (-ret));
      return NULL;
    }

  return cpu_buffer;
}

void key_callback (GLFWwindow* window, int key,
                   int scancode, int action, int mods)
{
  (void) scancode;
  (void) mods;

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

  self->window = glfwCreateWindow (width, height, "Vulkan Dmabuf", NULL, NULL);

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

  OpenVRVulkanClient *client = OPENVR_VULKAN_CLIENT (self->renderer);
  vkDeviceWaitIdle (client->device->device);

  return TRUE;
}

#define ALIGN(_v, _d) (((_v) + ((_d) - 1)) & ~((_d) - 1))

int
main () {
  Example example = {
    .should_quit = false
  };

  /* create dmabuf */
  uint32_t width = 1280;
  uint32_t height = 720;

  int fd;
  int stride = ALIGN ((int) width, 32) * 4;
  uint64_t size = (uint64_t) stride * height;
  char* map = (char*) allocate_dmabuf_amd (&example, size, &fd);

  dma_buf_fill (map, width, height, stride);

  init_glfw (&example, width, height);

  example.loop = g_main_loop_new (NULL, FALSE);

  example.renderer = openvr_vulkan_renderer_new ();

  uint32_t num_glfw_extensions = 0;
  const char** glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions (&num_glfw_extensions);

  GSList *instance_ext_list = NULL;
  for (uint32_t i = 0; i < num_glfw_extensions; i++)
    instance_ext_list = g_slist_append (instance_ext_list,
                                        (char*) glfw_extensions[i]);

  const gchar *device_extensions[] =
  {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,
    VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME
  };

  GSList *device_ext_list = NULL;
  for (uint32_t i = 0; i < G_N_ELEMENTS (device_extensions); i++)
    device_ext_list = g_slist_append (device_ext_list,
                                      (char*) device_extensions[i]);

  OpenVRVulkanClient *client = OPENVR_VULKAN_CLIENT (example.renderer);

  if (!openvr_vulkan_client_init_vulkan (client,
                                         instance_ext_list,
                                         device_ext_list,
                                         true))
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return -1;
  }

  if (glfwCreateWindowSurface (client->instance->instance,
                               example.window, NULL,
                              &example.surface) != VK_SUCCESS)
    {
      g_printerr ("Unable to create surface.\n");
      return -1;
    }

  example.texture = openvr_vulkan_texture_new ();

  if (!openvr_vulkan_client_load_dmabuf (client, example.texture,
                                         fd, width, height,
                                         VK_FORMAT_B8G8R8A8_UNORM))
    {
      g_printerr ("Unable to initialize vulkan dmabuf texture.\n");
      return -1;
    }

  if (!openvr_vulkan_renderer_init_rendering (example.renderer,
                                              example.surface,
                                              example.texture))
    return -1;

  g_timeout_add (20, draw_cb, &example);
  g_main_loop_run (example.loop);
  g_main_loop_unref (example.loop);

  g_slist_free (instance_ext_list);
  g_slist_free (device_ext_list);

  g_object_unref (example.texture);
  g_object_unref (example.renderer);

  glfwDestroyWindow (example.window);
  glfwTerminate ();

  int ret = amdgpu_bo_free(example.amd_bo);
  if (ret < 0)
    {
      g_printerr ("Could not free amdgpu buffer: %s\n", strerror (-ret));
      return -1;
    }

  ret = amdgpu_device_deinitialize (example.amd_dev);
  if (ret < 0)
    {
      g_printerr ("Could not free amdgpu device: %s\n", strerror (-ret));
      return -1;
    }

  return 0;
}
