/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-client.h"

G_DEFINE_TYPE (OpenVRVulkanClient, openvr_vulkan_client, G_TYPE_OBJECT)

static void
openvr_vulkan_client_finalize (GObject *gobject);

static void
openvr_vulkan_client_class_init (OpenVRVulkanClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_vulkan_client_finalize;
}

static void
openvr_vulkan_client_init (OpenVRVulkanClient *self)
{
  self->instance = openvr_vulkan_instance_new ();
  self->device = openvr_vulkan_device_new ();
  self->command_pool = VK_NULL_HANDLE;
}

OpenVRVulkanClient *
openvr_vulkan_client_new (void)
{
  return (OpenVRVulkanClient*) g_object_new (OPENVR_TYPE_VULKAN_CLIENT, 0);
}

static void
openvr_vulkan_client_finalize (GObject *gobject)
{
  OpenVRVulkanClient *self = OPENVR_VULKAN_CLIENT (gobject);

  vkDestroyCommandPool (self->device->device, self->command_pool, NULL);

  g_object_unref (self->device);
  g_object_unref (self->instance);
}

bool
openvr_vulkan_client_init_command_pool (OpenVRVulkanClient *self)
{
  VkCommandPoolCreateInfo command_pool_info =
    {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = self->device->queue_family_index,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    };

  VkResult res = vkCreateCommandPool (self->device->device,
                                     &command_pool_info,
                                      NULL, &self->command_pool);
  vk_check_error ("vkCreateCommandPool", res);
  return true;
}

bool
openvr_vulkan_client_begin_res_cmd_buffer (OpenVRVulkanClient  *self,
                                           FencedCommandBuffer *buffer)
{
  VkCommandBufferAllocateInfo command_buffer_info =
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandBufferCount = 1,
    .commandPool = self->command_pool,
  };

  VkResult res;
  res = vkAllocateCommandBuffers (self->device->device, &command_buffer_info,
                                 &buffer->cmd_buffer);
  vk_check_error ("vkAllocateCommandBuffers", res);

  VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
  };
  res = vkCreateFence (self->device->device, &fence_info,
                       NULL, &buffer->fence);
  vk_check_error ("vkCreateFence", res);

  VkCommandBufferBeginInfo command_buffer_begin_info =
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  };
  res = vkBeginCommandBuffer (buffer->cmd_buffer,
                             &command_buffer_begin_info);
  vk_check_error ("vkBeginCommandBuffer", res);

  return true;
}

bool
openvr_vulkan_client_submit_res_cmd_buffer (OpenVRVulkanClient  *self,
                                            FencedCommandBuffer *buffer)
{
  if (buffer == NULL || buffer->cmd_buffer == VK_NULL_HANDLE)
    {
      g_printerr ("Trying to submit empty FencedCommandBuffer\n.");
      return false;
    }

  VkResult res = vkEndCommandBuffer (buffer->cmd_buffer);
  vk_check_error ("vkEndCommandBuffer", res);

  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &buffer->cmd_buffer
  };

  res = vkQueueSubmit (self->device->queue, 1,
                      &submit_info, buffer->fence);
  vk_check_error ("vkQueueSubmit", res);

  vkQueueWaitIdle (self->device->queue);

  vkFreeCommandBuffers (self->device->device, self->command_pool,
                        1, &buffer->cmd_buffer);
  vkDestroyFence (self->device->device, buffer->fence, NULL);

  return true;
}

bool
openvr_vulkan_client_load_raw (OpenVRVulkanClient   *self,
                               OpenVRVulkanTexture  *texture,
                               guchar               *pixels,
                               guint                 width,
                               guint                 height,
                               gsize                 size,
                               VkFormat              format)
{
  FencedCommandBuffer buffer = {};
  if (!openvr_vulkan_client_begin_res_cmd_buffer (self, &buffer))
    return false;

  if (!openvr_vulkan_texture_from_pixels (texture,
                                          self->device,
                                          buffer.cmd_buffer,
                                          pixels, width, height, size, format))
    return false;

  if (!openvr_vulkan_client_submit_res_cmd_buffer (self, &buffer))
    return false;

  return true;
}

bool
openvr_vulkan_client_load_dmabuf (OpenVRVulkanClient   *self,
                                  OpenVRVulkanTexture  *texture,
                                  int                   fd,
                                  guint                 width,
                                  guint                 height,
                                  VkFormat              format)
{
  if (!openvr_vulkan_texture_from_dmabuf (texture, self->device,
                                          fd, width, height, format))
    return false;

  return true;
}

bool
openvr_vulkan_client_load_pixbuf (OpenVRVulkanClient   *self,
                                  OpenVRVulkanTexture  *texture,
                                  GdkPixbuf            *pixbuf)
{
  guint width = (guint) gdk_pixbuf_get_width (pixbuf);
  guint height = (guint) gdk_pixbuf_get_height (pixbuf);
  guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);
  gsize size = gdk_pixbuf_get_byte_length (pixbuf);

  return openvr_vulkan_client_load_raw (self, texture, pixels,
                                        width, height, size,
                                        VK_FORMAT_R8G8B8A8_UNORM);
}

bool
openvr_vulkan_client_load_cairo_surface (OpenVRVulkanClient  *self,
                                         OpenVRVulkanTexture *texture,
                                         cairo_surface_t     *surface)
{
  guint width = cairo_image_surface_get_width (surface);
  guint height = cairo_image_surface_get_height (surface);

  VkFormat format;
  cairo_format_t cr_format = cairo_image_surface_get_format (surface);
  switch (cr_format)
    {
    case CAIRO_FORMAT_ARGB32:
      format = VK_FORMAT_B8G8R8A8_UNORM;
      break;
    case CAIRO_FORMAT_RGB24:
    case CAIRO_FORMAT_A8:
    case CAIRO_FORMAT_A1:
    case CAIRO_FORMAT_RGB16_565:
    case CAIRO_FORMAT_RGB30:
      g_printerr ("Unsupported Cairo format\n");
      return FALSE;
    case CAIRO_FORMAT_INVALID:
      g_printerr ("Invalid Cairo format\n");
      return FALSE;
    default:
      g_printerr ("Unknown Cairo format\n");
      return FALSE;
    }

  guchar *pixels = cairo_image_surface_get_data (surface);

  int stride = cairo_image_surface_get_stride (surface);

  gsize size = stride * height;

  return openvr_vulkan_client_load_raw (self, texture, pixels,
                                        width, height, size, format);
}

bool
openvr_vulkan_client_init_vulkan (OpenVRVulkanClient *self,
                                  GSList             *instance_extensions,
                                  GSList             *device_extensions,
                                  bool                enable_validation)
{
  if (!openvr_vulkan_instance_create (self->instance,
                                      enable_validation,
                                      instance_extensions))
    {
      g_printerr ("Failed to create instance.\n");
      return false;
    }

  if (!openvr_vulkan_device_create (self->device, self->instance,
                                    VK_NULL_HANDLE, device_extensions))
    {
      g_printerr ("Failed to create device.\n");
      return false;
    }

  if (!openvr_vulkan_client_init_command_pool (self))
    {
      g_printerr ("Failed to create command pool.\n");
      return false;
    }

  return true;
}
