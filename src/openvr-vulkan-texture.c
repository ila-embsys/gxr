/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-texture.h"

#include <vulkan/vulkan.h>
#include <stdbool.h>

G_DEFINE_TYPE (OpenVRVulkanTexture, openvr_vulkan_texture, G_TYPE_OBJECT)

static void
openvr_vulkan_texture_init (OpenVRVulkanTexture *self)
{
  self->image = VK_NULL_HANDLE;
  self->image_memory = VK_NULL_HANDLE;
  self->image_view = VK_NULL_HANDLE;
  self->staging_buffer = VK_NULL_HANDLE;
  self->staging_buffer_memory = VK_NULL_HANDLE;
}

OpenVRVulkanTexture *
openvr_vulkan_texture_new (void)
{
  return (OpenVRVulkanTexture*) g_object_new (OPENVR_TYPE_VULKAN_TEXTURE, 0);
}

static void
openvr_vulkan_texture_finalize (GObject *gobject)
{
  OpenVRVulkanTexture *self = OPENVR_VULKAN_TEXTURE (gobject);
  VkDevice device = self->device->device;

  openvr_vulkan_texture_free_staging_memory (self);

  vkDestroyImageView (device, self->image_view, NULL);
  vkDestroyImage (device, self->image, NULL);
  vkFreeMemory (device, self->image_memory, NULL);
}

void
openvr_vulkan_texture_free_staging_memory (OpenVRVulkanTexture *self)
{
  VkDevice device = self->device->device;
  vkDestroyBuffer (device, self->staging_buffer, NULL);
  vkFreeMemory (device, self->staging_buffer_memory, NULL);
}

static void
openvr_vulkan_texture_class_init (OpenVRVulkanTextureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_vulkan_texture_finalize;
}

bool
openvr_vulkan_texture_allocate (OpenVRVulkanTexture *self,
                                OpenVRVulkanDevice  *device,
                                guint                width,
                                guint                height,
                                gsize                size,
                                VkFormat             format)
{
  self->width = width;
  self->height = height;
  self->device = device;

  VkImageCreateInfo image_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .extent.width = width,
    .extent.height = height,
    .extent.depth = 1,
    .mipLevels = 1,
    .arrayLayers = 1,
    .format = format,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .usage = VK_IMAGE_USAGE_SAMPLED_BIT |
             VK_IMAGE_USAGE_TRANSFER_DST_BIT |
             VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    .flags = 0
  };
  VkResult res;
  res = vkCreateImage (device->device, &image_info, NULL, &self->image);
  vk_check_error ("vkCreateImage", res);

  VkMemoryRequirements memory_requirements = {};
  vkGetImageMemoryRequirements (device->device, self->image,
                                &memory_requirements);

  VkMemoryAllocateInfo memory_info =
  {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memory_requirements.size
  };
  openvr_vulkan_device_memory_type_from_properties (
    device,
    memory_requirements.memoryTypeBits,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    &memory_info.memoryTypeIndex);
  res = vkAllocateMemory (device->device, &memory_info,
                          NULL, &self->image_memory);
  vk_check_error ("vkAllocateMemory", res);
  res = vkBindImageMemory (device->device, self->image, self->image_memory, 0);
  vk_check_error ("vkBindImageMemory", res);

  VkImageViewCreateInfo image_view_info =
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image = self->image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = image_info.format,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = image_info.mipLevels,
      .baseArrayLayer = 0,
      .layerCount = 1,
    }
  };
  res = vkCreateImageView (device->device, &image_view_info,
                           NULL, &self->image_view);
  vk_check_error ("vkCreateImageView", res);

  if (!openvr_vulkan_device_create_buffer (device,
                                           size,
                                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                          &self->staging_buffer,
                                          &self->staging_buffer_memory))
    return false;
  return true;
}

bool
openvr_vulkan_texture_upload_pixels (OpenVRVulkanTexture *self,
                                     VkCommandBuffer      cmd_buffer,
                                     guchar              *pixels,
                                     gsize                size)
{
  if (!openvr_vulkan_device_map_memory (self->device, pixels, size,
                                        self->staging_buffer_memory))
    return false;

  VkImageMemoryBarrier image_memory_barrier =
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .srcAccessMask = 0,
    .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    .image = self->image,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1, //image_info.mipLevels,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
    .srcQueueFamilyIndex = self->device->queue_family_index,
    .dstQueueFamilyIndex = self->device->queue_family_index
  };

  vkCmdPipelineBarrier (cmd_buffer,
                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                        &image_memory_barrier);

  VkBufferImageCopy buffer_image_copy = {
    .imageSubresource = {
      .baseArrayLayer = 0,
      .layerCount = 1,
      .mipLevel = 0,
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    },
    .imageExtent = {
      .width = self->width,
      .height = self->height,
      .depth = 1,
    }
  };

  vkCmdCopyBufferToImage (cmd_buffer,
                          self->staging_buffer, self->image,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          1,
                          &buffer_image_copy);

  image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  vkCmdPipelineBarrier (cmd_buffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0,
                        NULL, 1, &image_memory_barrier);
  return true;
}

bool
openvr_vulkan_texture_from_pixels (OpenVRVulkanTexture *self,
                                   OpenVRVulkanDevice  *device,
                                   VkCommandBuffer      cmd_buffer,
                                   guchar              *pixels,
                                   guint                width,
                                   guint                height,
                                   gsize                size,
                                   VkFormat             format)
{

  if (!openvr_vulkan_texture_allocate (self, device, width,
                                       height, size, format))
    return false;

  if (!openvr_vulkan_texture_upload_pixels (self, cmd_buffer, pixels, size))
    return false;
  return true;
}

bool
openvr_vulkan_texture_from_dmabuf (OpenVRVulkanTexture *self,
                                   OpenVRVulkanDevice  *device,
                                   int                  fd,
                                   guint                width,
                                   guint                height,
                                   VkFormat             format)
{
  self->width = width;
  self->height = height;
  self->device = device;

  VkImportMemoryFdInfoKHR import_memory_info = {
    .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
    .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
    .fd = fd
  };

  VkImageCreateInfo image_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .extent = {
      .width = width,
      .height = height,
      .depth = 1,
    },
    .mipLevels = 1,
    .arrayLayers = 1,
    .format = format,
    .tiling = VK_IMAGE_TILING_LINEAR,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    //.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED
  };
  VkResult res;
  res = vkCreateImage (device->device, &image_info, NULL, &self->image);
  vk_check_error ("vkCreateImage", res);

  VkMemoryRequirements memory_requirements = {};
  vkGetImageMemoryRequirements (device->device, self->image,
                                &memory_requirements);

  VkMemoryAllocateInfo memory_info =
  {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = &import_memory_info,
    .allocationSize = memory_requirements.size
  };

  openvr_vulkan_device_memory_type_from_properties (
    device,
    memory_requirements.memoryTypeBits,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    &memory_info.memoryTypeIndex);

  res = vkAllocateMemory (device->device, &memory_info,
                          NULL, &self->image_memory);
  vk_check_error ("vkAllocateMemory", res);

  res = vkBindImageMemory (device->device, self->image, self->image_memory, 0);
  vk_check_error ("vkBindImageMemory", res);

  VkImageViewCreateInfo image_view_info =
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .flags = 0,
    .image = self->image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = image_info.format,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = image_info.mipLevels,
      .baseArrayLayer = 0,
      .layerCount = 1,
    }
  };
  res = vkCreateImageView (device->device, &image_view_info,
                           NULL, &self->image_view);
  vk_check_error ("vkCreateImageView", res);

  return true;
}

void
openvr_vulkan_texture_transfer_layout (OpenVRVulkanTexture *self,
                                       OpenVRVulkanDevice  *device,
                                       VkCommandBuffer      cmd_buffer,
                                       VkImageLayout        old,
                                       VkImageLayout        new)
{
  VkImageMemoryBarrier image_memory_barrier =
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .srcAccessMask = 0,
    .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
    .oldLayout = old,
    .newLayout = new,
    .image = self->image,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
    .srcQueueFamilyIndex = device->queue_family_index,
    .dstQueueFamilyIndex = device->queue_family_index
  };

  vkCmdPipelineBarrier (cmd_buffer,
                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                        &image_memory_barrier);
}
