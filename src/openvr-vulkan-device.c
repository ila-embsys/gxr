/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-device.h"

G_DEFINE_TYPE (OpenVRVulkanDevice, openvr_vulkan_device, G_TYPE_OBJECT)

static void
openvr_vulkan_device_init (OpenVRVulkanDevice *self)
{
  self->device = VK_NULL_HANDLE;
  self->physical_device = VK_NULL_HANDLE;
  self->queue = VK_NULL_HANDLE;
}

OpenVRVulkanDevice *
openvr_vulkan_device_new (void)
{
  return (OpenVRVulkanDevice*) g_object_new (OPENVR_TYPE_VULKAN_DEVICE, 0);
}

static void
openvr_vulkan_device_finalize (GObject *gobject)
{
  OpenVRVulkanDevice *self = OPENVR_VULKAN_DEVICE (gobject);
  vkDestroyDevice (self->device, NULL);
}

static void
openvr_vulkan_device_class_init (OpenVRVulkanDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_vulkan_device_finalize;
}

bool
_find_physical_device (OpenVRVulkanDevice   *self,
                       OpenVRVulkanInstance *instance,
                       OpenVRSystem *system)
{
  uint32_t num_devices = 0;
  VkResult result =
    vkEnumeratePhysicalDevices (instance->instance, &num_devices, NULL);
  if (result != VK_SUCCESS || num_devices == 0)
    {
      g_printerr ("vkEnumeratePhysicalDevices failed, "
                  "unable to init and enumerate GPUs with Vulkan.\n");
      return false;
    }

  VkPhysicalDevice *physical_devices =
    g_malloc(sizeof(VkPhysicalDevice) * num_devices);
  result = vkEnumeratePhysicalDevices (instance->instance,
                                      &num_devices,
                                       physical_devices);
  if (result != VK_SUCCESS || num_devices == 0)
    {
      g_printerr ("vkEnumeratePhysicalDevices failed, "
                  "unable to init and enumerate GPUs with Vulkan.\n");
      return false;
    }

  /* Query OpenVR for the physical device to use */
  uint64_t physical_device = 0;
  system->functions->GetOutputDevice (&physical_device,
                                       ETextureType_TextureType_Vulkan,
                                       (struct VkInstance_T*) instance->instance);

  /* Select the physical device */
  self->physical_device = VK_NULL_HANDLE;
  for (uint32_t i = 0; i < num_devices; i++)
    if (((VkPhysicalDevice) physical_device) == physical_devices[i])
      {
        self->physical_device = (VkPhysicalDevice) physical_device;
        break;
      }

  if (self->physical_device == VK_NULL_HANDLE)
    {
      /* Fallback: Grab the first physical device */
      g_printerr ("Failed to find GetOutputDevice VkPhysicalDevice, "
                  "falling back to choosing first device.\n");
      self->physical_device = physical_devices[0];
    }
  g_free (physical_devices);

  vkGetPhysicalDeviceMemoryProperties (self->physical_device,
                                       &self->memory_properties);

  return true;
}

bool
_find_graphics_queue (OpenVRVulkanDevice *self)
{
  /* Find the first graphics queue */
  uint32_t num_queues = 0;
  vkGetPhysicalDeviceQueueFamilyProperties (
    self->physical_device, &num_queues, 0);

  VkQueueFamilyProperties *queue_family_props =
    g_malloc (sizeof(VkQueueFamilyProperties) * num_queues);

  vkGetPhysicalDeviceQueueFamilyProperties (
    self->physical_device, &num_queues, queue_family_props);

  if (num_queues == 0)
    {
      g_printerr ("Failed to get queue properties.\n");
      return false;
    }

  uint32_t i = 0;
  for (i = 0; i < num_queues; i++)
      if (queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
          break;

  if (i >= num_queues)
    {
      g_printerr ("No graphics queue found\n");
      return false;
    }
  self->queue_family_index = i;

  g_free (queue_family_props);

  return true;
}

bool
_get_device_extension_count (OpenVRVulkanDevice *self,
                             uint32_t           *count)
{
  VkResult result =
    vkEnumerateDeviceExtensionProperties (self->physical_device, NULL,
                                          count, NULL);
  if (result != VK_SUCCESS)
  {
    g_printerr ("vkEnumerateDeviceExtensionProperties failed with error %d\n",
                result);
    return false;
  }
  return true;
}

bool
_init_device_extensions (OpenVRVulkanDevice *self,
                         OpenVRCompositor   *compositor,
                         uint32_t            num_extensions,
                         const char        **extension_names,
                         uint32_t           *out_num_enabled)
{
  uint32_t num_enabled = 0;

  /* Enable required device extensions */
  VkExtensionProperties *extension_props =
    g_malloc(sizeof(VkExtensionProperties) * num_extensions);

  memset (extension_props, 0, sizeof (VkExtensionProperties) * num_extensions);

  VkResult result = vkEnumerateDeviceExtensionProperties (self->physical_device,
                                                          NULL,
                                                         &num_extensions,
                                                          extension_props);
  if (result != VK_SUCCESS)
    {
      g_printerr ("vkEnumerateDeviceExtensionProperties"
                  " failed with error %d\n",
                  result);
      return false;
    }

  /* Query required OpenVR device extensions */
  GSList *required_extensions = NULL;
  openvr_compositor_get_device_extensions (compositor,
                                           self->physical_device,
                                          &required_extensions);

  for (uint32_t i = 0; i < g_slist_length (required_extensions); i++)
    {
      bool found = false;
      for (uint32_t j = 0; j < num_extensions; j++)
        {
          GSList* extension_name = g_slist_nth (required_extensions, i);
          if (strcmp ((gchar*) extension_name->data,
                      extension_props[j].extensionName) == 0)
            {
              found = true;
              break;
            }
        }

      if (found)
        {
          GSList* extension_name = g_slist_nth (required_extensions, i);
          extension_names[num_enabled] = (gchar*) extension_name->data;
          num_enabled++;
        }
    }

  *out_num_enabled = num_enabled;

  return true;
}

bool
openvr_vulkan_device_create (OpenVRVulkanDevice   *self,
                             OpenVRVulkanInstance *instance,
                             OpenVRSystem         *system,
                             OpenVRCompositor     *compositor)
{
  if (!_find_physical_device (self, instance, system))
    return false;

  if (!_find_graphics_queue (self))
    return false;

  uint32_t num_extensions = 0;
  if (!_get_device_extension_count(self, &num_extensions))
    return false;

  const char **extension_names =
    g_malloc(sizeof(const char *) * num_extensions);
  uint32_t num_enabled = 0;

  if (num_extensions > 0)
    if (!_init_device_extensions (self, compositor, num_extensions,
                                  extension_names, &num_enabled))
      return false;

  g_print ("Requesting device extensions:\n");
  for (int i = 0; i < num_enabled; i++)
      g_print ("%s\n", extension_names[i]);

  /* Create the device */
  float queue_priority = 1.0f;

  VkPhysicalDeviceFeatures physical_device_features;
  vkGetPhysicalDeviceFeatures (self->physical_device,
                              &physical_device_features);

  VkDeviceCreateInfo device_info =
    {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &(VkDeviceQueueCreateInfo)
        {
          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
          .queueFamilyIndex = self->queue_family_index,
          .queueCount = 1,
          .pQueuePriorities = &queue_priority
        },
      .enabledExtensionCount = num_enabled,
      .ppEnabledExtensionNames = extension_names,
      .pEnabledFeatures = &physical_device_features
    };

  VkResult result = vkCreateDevice (self->physical_device,
                                   &device_info, NULL, &self->device);
  if (result != VK_SUCCESS)
    {
      g_printerr ("vkCreateDevice failed with error %d\n", result);
      return false;
    }

  /* Get the device queue */
  vkGetDeviceQueue (self->device, self->queue_family_index, 0, &self->queue);
  return true;
}

/*
 * Determine the memory type index from the memory requirements and type bits
 */
bool
openvr_vulkan_device_memory_type_from_properties (
  OpenVRVulkanDevice   *self,
  uint32_t              memory_type_bits,
  VkMemoryPropertyFlags memory_property_flags,
  uint32_t             *type_index_out)
{
  for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
  {
    if ((memory_type_bits & 1) == 1)
    {
      /* Type is available, does it match user properties? */
      if ((self->memory_properties.memoryTypes[i].propertyFlags
           & memory_property_flags) == memory_property_flags)
      {
        *type_index_out = i;
        return true;
      }
    }
    memory_type_bits >>= 1;
  }

  /* No memory types matched, return failure */
  return false;
}

/* Helper function to create Vulkan static VB/IBs */
bool
openvr_vulkan_device_create_buffer (OpenVRVulkanDevice *self,
                                    const void         *buffer_data,
                                    VkDeviceSize        size,
                                    VkBufferUsageFlags  usage,
                                    VkBuffer           *buffer_out,
                                    VkDeviceMemory     *device_memory_out)
{
  /* Create the vertex buffer and fill with data */
  VkBufferCreateInfo buffer_info =
  {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = usage,
  };
  VkResult result =
    vkCreateBuffer (self->device, &buffer_info, NULL, buffer_out);
  if (result != VK_SUCCESS)
  {
    g_printerr ("vkCreateBuffer failed with error %d\n", result);
    return false;
  }

  VkMemoryRequirements memory_requirements = {};
  vkGetBufferMemoryRequirements (self->device,
                                *buffer_out,
                                 &memory_requirements);

  VkMemoryAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memory_requirements.size
  };
  if (!openvr_vulkan_device_memory_type_from_properties (
        self,
        memory_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &alloc_info.memoryTypeIndex))
  {
    g_printerr ("Failed to find matching memoryTypeIndex for buffer\n");
    return false;
  }

  result =
    vkAllocateMemory (self->device, &alloc_info, NULL, device_memory_out);
  if (result != VK_SUCCESS)
  {
    g_printerr ("vkCreateBuffer failed with error %d\n", result);
    return false;
  }

  result =
    vkBindBufferMemory (self->device, *buffer_out, *device_memory_out, 0);
  if (result != VK_SUCCESS)
  {
    g_printerr ("vkBindBufferMemory failed with error %d\n", result);
    return false;
  }

  if (buffer_data != NULL)
  {
    void *data;
    result = vkMapMemory (self->device,
                          *device_memory_out,
                          0, VK_WHOLE_SIZE, 0, &data);
    if (result != VK_SUCCESS)
    {
      g_printerr ("vkMapMemory returned error %d\n", result);
      return false;
    }
    memcpy (data, buffer_data, size);
    vkUnmapMemory (self->device, *device_memory_out);

    VkMappedMemoryRange memory_range = {
      .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
      .memory = *device_memory_out,
      .size = VK_WHOLE_SIZE
    };
    vkFlushMappedMemoryRanges (self->device, 1, &memory_range);
  }
  return true;
}
