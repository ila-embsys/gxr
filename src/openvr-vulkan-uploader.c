/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 *
 * Based on openvr hellovr example.
 */

#include "openvr-vulkan-uploader.h"
#include "openvr-global.h"

#include "openvr_capi_global.h"

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#include <gmodule.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "openvr-overlay.h"

G_DEFINE_TYPE (OpenVRVulkanUploader, openvr_vulkan_uploader, G_TYPE_OBJECT)

static void
openvr_vulkan_uploader_finalize (GObject *gobject);


VulkanCommandBuffer_t*
_get_command_buffer (OpenVRVulkanUploader *self);

bool
_init_device (OpenVRVulkanUploader *self);

static void
openvr_vulkan_uploader_class_init (OpenVRVulkanUploaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_vulkan_uploader_finalize;
}

static void
openvr_vulkan_uploader_init (OpenVRVulkanUploader *self)
{
  self->instance = openvr_vulkan_instance_new ();
  self->device = openvr_vulkan_device_new ();
  self->texture = openvr_vulkan_texture_new ();
  self->command_pool = VK_NULL_HANDLE;
  self->cmd_buffers = g_queue_new ();
}

OpenVRVulkanUploader *
openvr_vulkan_uploader_new (void)
{
  return (OpenVRVulkanUploader*) g_object_new (OPENVR_TYPE_VULKAN_UPLOADER, 0);
}

void
_cleanup_command_buffer_queue (gpointer item, OpenVRVulkanUploader *self)
{
  VulkanCommandBuffer_t *b = (VulkanCommandBuffer_t*) item;
  vkFreeCommandBuffers (self->device->device,
                        self->command_pool, 1, &b->cmd_buffer);
  vkDestroyFence (self->device->device, b->fence, NULL);
  // g_object_unref (item);
}

static void
openvr_vulkan_uploader_finalize (GObject *gobject)
{
  OpenVRVulkanUploader *self = OPENVR_VULKAN_UPLOADER (gobject);

  /* Idle the device to make sure no work is outstanding */
  if (self->device != VK_NULL_HANDLE)
    vkDeviceWaitIdle (self->device->device);

  if (self->device != VK_NULL_HANDLE)
  {
    g_queue_foreach (self->cmd_buffers,
                     (GFunc) _cleanup_command_buffer_queue, self);

    vkDestroyCommandPool (self->device->device, self->command_pool, NULL);

    g_object_unref (self->texture);

    g_object_unref (self->device);

    g_object_unref (self->instance);
  }

  G_OBJECT_CLASS (openvr_vulkan_uploader_parent_class)->finalize (gobject);
}

bool
_init_command_pool (OpenVRVulkanUploader *self)
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
_begin_command_buffer (OpenVRVulkanUploader *self)
{
  /* Command buffer used during resource loading */
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
_submit_command_buffer (OpenVRVulkanUploader *self)
{
  /* Submit the command buffer used during loading */
  vkEndCommandBuffer (self->current_cmd_buffer->cmd_buffer);

  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &self->current_cmd_buffer->cmd_buffer
  };

  vkQueueSubmit (self->device->queue, 1, &submit_info, self->current_cmd_buffer->fence);
  g_queue_push_head (self->cmd_buffers, (gpointer) self->current_cmd_buffer);

  /* Wait for the GPU before proceeding */
  vkQueueWaitIdle (self->device->queue);
}

bool
openvr_vulkan_uploader_load_texture_raw (OpenVRVulkanUploader *self,
                                         guchar               *pixels,
                                         guint                 width,
                                         guint                 height,
                                         gsize                 size)
{
  _begin_command_buffer (self);

  if (!openvr_vulkan_texture_from_pixels (self->texture,
                                          self->device,
                                          self->current_cmd_buffer->cmd_buffer,
                                          pixels, width, height, size))
    return false;

  _submit_command_buffer (self);

  return true;
}

bool
openvr_vulkan_uploader_init_vulkan (OpenVRVulkanUploader *self,
                                    bool enable_validation,
                                    OpenVRSystem *system,
                                    OpenVRCompositor *compositor)
{
  if (!openvr_vulkan_instance_create (self->instance,
                                      enable_validation,
                                      compositor))
    {
      g_printerr ("Failed to create instance.\n");
      return false;
    }

  if (!openvr_vulkan_device_create (self->device,
                                    self->instance,
                                    system,
                                    compositor))
    {
      g_printerr ("Failed to create device.\n");
      return false;
    }

  if(!_init_command_pool (self))
    {
      g_printerr ("Failed to create command pool.\n");
      return false;
    }

  return true;
}

void
openvr_vulkan_uploader_submit_frame (OpenVRVulkanUploader *self,
                                     OpenVROverlay        *overlay)
{
  /* Submit to SteamVR */
  struct VRVulkanTextureData_t texture_data =
    {
      .m_nImage = (uint64_t) self->texture->image,
      .m_pDevice = (struct VkDevice_T*) self->device->device,
      .m_pPhysicalDevice = (struct VkPhysicalDevice_T*)
        self->device->physical_device,
      .m_pInstance = (struct VkInstance_T*) self->instance->instance,
      .m_pQueue = (struct VkQueue_T*) self->device->queue,
      .m_nQueueFamilyIndex = self->device->queue_family_index,
      .m_nWidth = self->width,
      .m_nHeight = self->height,
      .m_nFormat = VK_FORMAT_B8G8R8A8_UNORM,
      .m_nSampleCount = 1
    };

  struct Texture_t texture =
    {
      .handle = &texture_data,
      .eType = ETextureType_TextureType_Vulkan,
      .eColorSpace = EColorSpace_ColorSpace_Auto
    };

  overlay->functions->SetOverlayTexture (overlay->overlay_handle, &texture);
}

/*
 * Get an available command buffer or create a new one if one available.
 * Associate a fence with the command buffer.
 */
VulkanCommandBuffer_t*
_get_command_buffer (OpenVRVulkanUploader *self)
{
  VulkanCommandBuffer_t *command_buffer = g_new (VulkanCommandBuffer_t, 1);

  if (g_queue_get_length (self->cmd_buffers) > 0)
  {
    /*
     * If the fence associated with the command buffer has finished,
     * reset it and return it
     */
    VulkanCommandBuffer_t *tail =
      (VulkanCommandBuffer_t*) g_queue_peek_tail(self->cmd_buffers);

    if (vkGetFenceStatus (self->device->device, tail->fence) == VK_SUCCESS)
    {
      command_buffer->cmd_buffer = tail->cmd_buffer;
      command_buffer->fence = tail->fence;

      vkResetCommandBuffer (command_buffer->cmd_buffer,
                            VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
      vkResetFences (self->device->device, 1, &command_buffer->fence);
      gpointer last = g_queue_pop_tail (self->cmd_buffers);
      g_object_unref (last);
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
