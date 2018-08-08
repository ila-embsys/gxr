/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_VULKAN_DEVICE_H_
#define OPENVR_GLIB_VULKAN_DEVICE_H_

#include <glib-object.h>

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "openvr-vulkan-instance.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_VULKAN_DEVICE openvr_vulkan_device_get_type()
G_DECLARE_FINAL_TYPE (OpenVRVulkanDevice, openvr_vulkan_device, OPENVR, VULKAN_DEVICE, GObject)

struct _OpenVRVulkanDevice
{
  GObjectClass parent_class;

  VkDevice device;
  VkPhysicalDevice physical_device;

  VkQueue queue;

  uint32_t queue_family_index;

  VkPhysicalDeviceMemoryProperties memory_properties;
};

OpenVRVulkanDevice *openvr_vulkan_device_new (void);

bool
openvr_vulkan_device_create (OpenVRVulkanDevice   *self,
                             OpenVRVulkanInstance *instance,
                             VkPhysicalDevice      device,
                             GSList               *extensions);

bool
openvr_vulkan_device_memory_type_from_properties (
  OpenVRVulkanDevice   *self,
  uint32_t              memory_type_bits,
  VkMemoryPropertyFlags memory_property_flags,
  uint32_t             *type_index_out);

bool
openvr_vulkan_device_create_buffer (OpenVRVulkanDevice   *self,
                                    VkDeviceSize          size,
                                    VkBufferUsageFlags    usage,
                                    VkMemoryPropertyFlags properties,
                                    VkBuffer             *buffer,
                                    VkDeviceMemory       *memory);

bool
openvr_vulkan_device_map_memory (OpenVRVulkanDevice *self,
                                 const void         *data,
                                 VkDeviceSize        size,
                                 VkDeviceMemory      memory);

bool
openvr_vulkan_device_queue_supports_surface (OpenVRVulkanDevice *self,
                                             VkSurfaceKHR surface);

G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_DEVICE_H_ */
