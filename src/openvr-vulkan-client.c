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

VulkanCommandBuffer_t*
_get_command_buffer (OpenVRVulkanClient *self);

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
  self->cmd_buffers = g_queue_new ();
}

OpenVRVulkanClient *
openvr_vulkan_client_new (void)
{
  return (OpenVRVulkanClient*) g_object_new (OPENVR_TYPE_VULKAN_CLIENT, 0);
}

void
_cleanup_command_buffer_queue (gpointer item, OpenVRVulkanClient *self)
{
  VulkanCommandBuffer_t *b = (VulkanCommandBuffer_t*) item;
  vkFreeCommandBuffers (self->device->device,
                        self->command_pool, 1, &b->cmd_buffer);
  vkDestroyFence (self->device->device, b->fence, NULL);
  g_free (item);
}

static void
openvr_vulkan_client_finalize (GObject *gobject)
{
  OpenVRVulkanClient *self = OPENVR_VULKAN_CLIENT (gobject);

  if (self->device->device != VK_NULL_HANDLE)
  {
    g_queue_foreach (self->cmd_buffers,
                     (GFunc) _cleanup_command_buffer_queue, self);

    vkDestroyCommandPool (self->device->device, self->command_pool, NULL);

    g_object_unref (self->device);
    g_object_unref (self->instance);
  }
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

  VkResult result = vkCreateCommandPool (self->device->device,
                                         &command_pool_info,
                                         NULL, &self->command_pool);
  if (result != VK_SUCCESS)
    {
      g_printerr ("vkCreateCommandPool returned error %d.", result);
      return false;
    }
  return true;
}

void
openvr_vulkan_client_begin_res_cmd_buffer (OpenVRVulkanClient *self)
{
  self->current_cmd_buffer = _get_command_buffer (self);
  VkCommandBufferBeginInfo command_buffer_begin_info =
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  };
  vkBeginCommandBuffer (self->current_cmd_buffer->cmd_buffer,
                        &command_buffer_begin_info);
}

void
openvr_vulkan_client_submit_res_cmd_buffer (OpenVRVulkanClient *self)
{
  vkEndCommandBuffer (self->current_cmd_buffer->cmd_buffer);

  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &self->current_cmd_buffer->cmd_buffer
  };

  vkQueueSubmit (self->device->queue, 1,
                 &submit_info, self->current_cmd_buffer->fence);
  g_queue_push_head (self->cmd_buffers, (gpointer) self->current_cmd_buffer);

  vkQueueWaitIdle (self->device->queue);
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
  openvr_vulkan_client_begin_res_cmd_buffer (self);

  if (!openvr_vulkan_texture_from_pixels (texture,
                                          self->device,
                                          self->current_cmd_buffer->cmd_buffer,
                                          pixels, width, height, size, format))
    return false;

  openvr_vulkan_client_submit_res_cmd_buffer (self);

  openvr_vulkan_texture_free_staging_memory (texture);

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

/*
 * Get an available command buffer or create a new one if one available.
 * Associate a fence with the command buffer.
 */
VulkanCommandBuffer_t*
_get_command_buffer (OpenVRVulkanClient *self)
{
  VulkanCommandBuffer_t *command_buffer = g_new (VulkanCommandBuffer_t, 1);

  if (g_queue_get_length (self->cmd_buffers) > 0)
  {
    /*
     * If the fence associated with the command buffer has finished,
     * reset it and return it
     */
    VulkanCommandBuffer_t *tail =
      (VulkanCommandBuffer_t*) g_queue_peek_tail (self->cmd_buffers);

    if (vkGetFenceStatus (self->device->device, tail->fence) == VK_SUCCESS)
    {
      command_buffer->cmd_buffer = tail->cmd_buffer;
      command_buffer->fence = tail->fence;

      vkResetCommandBuffer (command_buffer->cmd_buffer,
                            VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
      vkResetFences (self->device->device, 1, &command_buffer->fence);
      gpointer last = g_queue_pop_tail (self->cmd_buffers);
      g_free (last);

      return command_buffer;
    }
  }

  /* Create a new command buffer and associated fence */
  VkCommandBufferAllocateInfo command_buffer_info =
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandBufferCount = 1,
    .commandPool = self->command_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
  };
  vkAllocateCommandBuffers (self->device->device, &command_buffer_info,
                            &command_buffer->cmd_buffer);

  VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
  };
  vkCreateFence (self->device->device, &fence_info,
                 NULL, &command_buffer->fence);
  return command_buffer;
}
