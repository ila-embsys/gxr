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
openvr_vulkan_client_upload_pixels (OpenVRVulkanClient   *self,
                                    OpenVRVulkanTexture  *texture,
                                    guchar               *pixels,
                                    gsize                 size)
{
  FencedCommandBuffer buffer = {};
  if (!openvr_vulkan_client_begin_res_cmd_buffer (self, &buffer))
    return false;

  if (!openvr_vulkan_texture_upload_pixels (texture, buffer.cmd_buffer,
                                            pixels, size))
    return false;

  if (!openvr_vulkan_client_submit_res_cmd_buffer (self, &buffer))
    return false;

  return true;
}

bool
openvr_vulkan_client_upload_pixbuf (OpenVRVulkanClient   *self,
                                    OpenVRVulkanTexture  *texture,
                                    GdkPixbuf            *pixbuf)
{
  guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);
  gsize size = gdk_pixbuf_get_byte_length (pixbuf);

  return openvr_vulkan_client_upload_pixels (self, texture, pixels, size);
}

bool
openvr_vulkan_client_transfer_layout (OpenVRVulkanClient  *self,
                                      OpenVRVulkanTexture *texture,
                                      VkImageLayout        old,
                                      VkImageLayout        new)
{
  FencedCommandBuffer cmd_buffer = {};
  if (!openvr_vulkan_client_begin_res_cmd_buffer (self, &cmd_buffer))
    return false;

  openvr_vulkan_texture_transfer_layout (texture, self->device,
                                         cmd_buffer.cmd_buffer, old, new);

  if (!openvr_vulkan_client_submit_res_cmd_buffer (self, &cmd_buffer))
    return false;

  return true;
}

bool
openvr_vulkan_client_upload_cairo_surface (OpenVRVulkanClient  *self,
                                           OpenVRVulkanTexture *texture,
                                           cairo_surface_t     *surface)
{
  guchar *pixels = cairo_image_surface_get_data (surface);
  gsize size = cairo_image_surface_get_stride (surface) *
               cairo_image_surface_get_height (surface);

  return openvr_vulkan_client_upload_pixels (self, texture, pixels, size);
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
