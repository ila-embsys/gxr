/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 *
 * Based on openvr hellovr example.
 */

#include "openvr-vulkan-uploader.h"

#include "openvr_capi_global.h"

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#include <gmodule.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifndef _countof
#define _countof(x) (sizeof (x) / sizeof ((x)[0]))
#endif

G_DEFINE_TYPE (OpenVRVulkanUploader, openvr_vulkan_uploader, G_TYPE_OBJECT)

static void
openvr_vulkan_uploader_class_init (OpenVRVulkanUploaderClass *klass) {}

static void
openvr_vulkan_uploader_init (OpenVRVulkanUploader *self)
{
  self->enable_validation = false;
  self->instance = VK_NULL_HANDLE;
  self->device = VK_NULL_HANDLE;
  self->physical_device = VK_NULL_HANDLE;
  self->queue = VK_NULL_HANDLE;
  self->debug_report_cb = VK_NULL_HANDLE;
  self->command_pool = VK_NULL_HANDLE;
  self->image = VK_NULL_HANDLE;
  self->image_memory = VK_NULL_HANDLE;
  self->image_view = VK_NULL_HANDLE;
  self->staging_buffer = VK_NULL_HANDLE;
  self->staging_buffer_memory = VK_NULL_HANDLE;
  self->cmd_buffers = g_queue_new ();
  self->system = NULL;
  self->compositor = NULL;
  self->overlay = NULL;
}

OpenVRVulkanUploader *
openvr_vulkan_uploader_new (void)
{
  return (OpenVRVulkanUploader*) g_object_new (OPENVR_TYPE_VULKAN_UPLOADER, 0);
}

gboolean _init_fn_table (const char *type, intptr_t *ret)
{
  EVRInitError error;
  char fn_table_name[128];
  g_sprintf (fn_table_name, "FnTable:%s", type);

  *ret = VR_GetGenericInterface (fn_table_name, &error);

  if (error != EVRInitError_VRInitError_None)
    {
      g_printerr ("compositor: VR_GetGenericInterface returned error: %s.\n",
                  VR_GetVRInitErrorAsSymbol (error));
      return false;
    }
  return true;
}

gboolean _system_init_fn_table2 (OpenVRVulkanUploader *self)
{
  intptr_t ptr = 0;
  gboolean ret = _init_fn_table (IVRSystem_Version, &ptr);
  if (!ret || ptr == 0)
    return false;

  self->system = (struct VR_IVRSystem_FnTable*) ptr;
  return true;
}

gboolean _overlay_init_fn_table2 (OpenVRVulkanUploader *self)
{
  intptr_t ptr = 0;
  gboolean ret = _init_fn_table (IVROverlay_Version, &ptr);
  if (!ret || ptr == 0)
    return false;

  self->overlay = (struct VR_IVROverlay_FnTable*) ptr;
  return true;
}

gboolean _compositor_init_fn_table (OpenVRVulkanUploader *self)
{
  intptr_t ptr = 0;
  gboolean ret = _init_fn_table (IVRCompositor_Version, &ptr);
  if (!ret || ptr == 0)
    return false;

  self->compositor = (struct VR_IVRCompositor_FnTable*) ptr;
  return true;
}

/* Vulkan extension entrypoints */
static PFN_vkCreateDebugReportCallbackEXT
  g_pVkCreateDebugReportCallbackEXT = NULL;
static PFN_vkDestroyDebugReportCallbackEXT
  g_pVkDestroyDebugReportCallbackEXT = NULL;

#define ENUM_TO_STR(r) case r: return #r

static const gchar*
debug_report_string (VkDebugReportFlagBitsEXT code)
{
  switch (code) {
    ENUM_TO_STR(VK_DEBUG_REPORT_INFORMATION_BIT_EXT);
    ENUM_TO_STR(VK_DEBUG_REPORT_WARNING_BIT_EXT);
    ENUM_TO_STR(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
    ENUM_TO_STR(VK_DEBUG_REPORT_ERROR_BIT_EXT);
    ENUM_TO_STR(VK_DEBUG_REPORT_DEBUG_BIT_EXT);
    ENUM_TO_STR(VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT);
    default:
      return "UNKNOWN DEBUG_REPORT";
  }
}

static const gchar*
result_string(VkResult code) {
  switch (code) {
    ENUM_TO_STR(VK_SUCCESS);
    ENUM_TO_STR(VK_NOT_READY);
    ENUM_TO_STR(VK_TIMEOUT);
    ENUM_TO_STR(VK_EVENT_SET);
    ENUM_TO_STR(VK_EVENT_RESET);
    ENUM_TO_STR(VK_INCOMPLETE);
    ENUM_TO_STR(VK_ERROR_OUT_OF_HOST_MEMORY);
    ENUM_TO_STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
    ENUM_TO_STR(VK_ERROR_INITIALIZATION_FAILED);
    ENUM_TO_STR(VK_ERROR_DEVICE_LOST);
    ENUM_TO_STR(VK_ERROR_MEMORY_MAP_FAILED);
    ENUM_TO_STR(VK_ERROR_LAYER_NOT_PRESENT);
    ENUM_TO_STR(VK_ERROR_EXTENSION_NOT_PRESENT);
    ENUM_TO_STR(VK_ERROR_FEATURE_NOT_PRESENT);
    ENUM_TO_STR(VK_ERROR_INCOMPATIBLE_DRIVER);
    ENUM_TO_STR(VK_ERROR_TOO_MANY_OBJECTS);
    ENUM_TO_STR(VK_ERROR_FORMAT_NOT_SUPPORTED);
    ENUM_TO_STR(VK_ERROR_SURFACE_LOST_KHR);
    ENUM_TO_STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
    ENUM_TO_STR(VK_SUBOPTIMAL_KHR);
    ENUM_TO_STR(VK_ERROR_OUT_OF_DATE_KHR);
    ENUM_TO_STR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
    ENUM_TO_STR(VK_ERROR_VALIDATION_FAILED_EXT);
    ENUM_TO_STR(VK_ERROR_INVALID_SHADER_NV);
    default:
      return "UNKNOWN RESULT";
  }
}

static VkBool32 VKAPI_PTR
validation_cb (
  VkDebugReportFlagsEXT      flags,
  VkDebugReportObjectTypeEXT object_type,
  uint64_t                   object,
  size_t                     location,
  int32_t                    message_code,
  const char                *layer_prefix,
  const char                *message,
  void                      *user_data)
{
  g_print ("%s %s %lu:%d: %s\n",
           debug_report_string (flags),
           layer_prefix, location, message_code, message);
  return VK_FALSE;
}

/*
 * Determine the memory type index from the memory requirements and type bits
 */
static bool
memory_type_from_properties (
  VkPhysicalDeviceMemoryProperties *memory_properties,
  uint32_t                          memory_type_bits,
  VkMemoryPropertyFlags             memory_property_flags,
  uint32_t                         *type_index_out)
{
  for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
  {
    if ((memory_type_bits & 1) == 1)
    {
      /* Type is available, does it match user properties? */
      if ((memory_properties->memoryTypes[i].propertyFlags
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
static bool
create_vulkan_buffer (
  VkDevice                          device,
  VkPhysicalDeviceMemoryProperties *memory_properties,
  const void                       *buffer_data,
  VkDeviceSize                      size,
  VkBufferUsageFlags                usage,
  VkBuffer                         *buffer_out,
  VkDeviceMemory                   *device_memory_out)
{
  /* Create the vertex buffer and fill with data */
  VkBufferCreateInfo buffer_info =
  {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = usage,
  };
  VkResult result = vkCreateBuffer (device, &buffer_info, NULL, buffer_out);
  if (result != VK_SUCCESS)
  {
    g_printerr ("vkCreateBuffer failed with error %d\n", result);
    return false;
  }

  VkMemoryRequirements memory_requirements = {};
  vkGetBufferMemoryRequirements (device, *buffer_out, &memory_requirements);

  VkMemoryAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memory_requirements.size
  };
  if (!memory_type_from_properties (memory_properties,
                                    memory_requirements.memoryTypeBits,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                   &alloc_info.memoryTypeIndex))
  {
    g_printerr ("Failed to find matching memoryTypeIndex for buffer\n");
    return false;
  }

  result = vkAllocateMemory (device, &alloc_info, NULL, device_memory_out);
  if (result != VK_SUCCESS)
  {
    g_printerr ("vkCreateBuffer failed with error %d\n", result);
    return false;
  }

  result = vkBindBufferMemory (device, *buffer_out, *device_memory_out, 0);
  if (result != VK_SUCCESS)
  {
    g_printerr ("vkBindBufferMemory failed with error %d\n", result);
    return false;
  }

  if (buffer_data != NULL)
  {
    void *data;
    result =
      vkMapMemory (device, *device_memory_out, 0, VK_WHOLE_SIZE, 0, &data);
    if (result != VK_SUCCESS)
    {
      g_printerr ("vkMapMemory returned error %d\n", result);
      return false;
    }
    memcpy (data, buffer_data, size);
    vkUnmapMemory (device, *device_memory_out);

    VkMappedMemoryRange memory_range = {
      .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
      .memory = *device_memory_out,
      .size = VK_WHOLE_SIZE
    };
    vkFlushMappedMemoryRanges (device, 1, &memory_range);
  }
  return true;
}

bool
openvr_vulkan_uploader_init_openvr (OpenVRVulkanUploader *self)
{
  /* Loading the SteamVR Runtime */

  EVRInitError error;
  VR_InitInternal(&error, EVRApplicationType_VRApplication_Overlay);

  if (error != EVRInitError_VRInitError_None) {
    g_printerr ("Could not init OpenVR runtime: Error code %s\n",
                VR_GetVRInitErrorAsSymbol (error));
    return false;
  }

  _system_init_fn_table2 (self);

  if (!openvr_vulkan_uploader_init_compositor (self))
  {
    g_printerr ("Failed to initialize VR Compositor!\n");
    return false;
  }

  if (!openvr_vulkan_uploader_init_vulkan (self))
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return false;
  }

  if (_overlay_init_fn_table2 (self))
  {
    EVROverlayError err = self->overlay->CreateDashboardOverlay (
      "vulkan-overlay", "vkovl",
      &self->overlay_handle, &self->thumbnail_handle);

    if (err != EVROverlayError_VROverlayError_None)
      {
        g_printerr ("Could not create overlay: %s\n",
                    self->overlay->GetOverlayErrorNameFromEnum (err));
        return false;
      }
  } else {
    return false;
  }

  return true;
}

void
_split (gchar *str, GSList **out_list)
{
  gchar **array = g_strsplit (str, " ", 0);
  int i = 0;
  while(array[i] != NULL)
    {
      *out_list = g_slist_append (*out_list, g_strdup (array[i]));
      i++;
    }
  g_strfreev (array);
}

/* Ask OpenVR for the list of instance extensions required */
void
_get_required_instance_extensions (OpenVRVulkanUploader *self,
                                   GSList              **out_list)
{
  uint32_t size =
    self->compositor->GetVulkanInstanceExtensionsRequired (NULL, 0);
  if (size > 0)
  {
    gchar *all_extensions = g_malloc(sizeof(gchar) * size);
    all_extensions[0] = 0;
    self->compositor->GetVulkanInstanceExtensionsRequired (all_extensions,
                                                           size);
    _split (all_extensions, out_list);
    g_free(all_extensions);
  }
}

/* Ask OpenVR for the list of device extensions required */
void
_get_required_device_extensions (OpenVRVulkanUploader *self,
                                 VkPhysicalDevice      physical_device,
                                 GSList              **out_list)
{
  uint32_t size = self->compositor->GetVulkanDeviceExtensionsRequired (
    (struct VkPhysicalDevice_T*) physical_device, NULL, 0);

  if (size > 0)
  {
    gchar *all_extensions = g_malloc(sizeof(gchar) * size);
    all_extensions[0] = 0;
    self->compositor->GetVulkanDeviceExtensionsRequired (
      (struct VkPhysicalDevice_T*) physical_device, all_extensions, size);

    _split (all_extensions, out_list);
    g_free (all_extensions);
  }
}

bool
_init_validation_layers (const char **enabled_layer_names,
                         uint32_t    *num_enabled_layers,
                         GSList     **required_extensions)
{
  uint32_t num_layers = 0;
  VkResult result = vkEnumerateInstanceLayerProperties (&num_layers, NULL);
  if (result == VK_SUCCESS && num_layers > 0)
  {
    VkLayerProperties * layer_props =
      g_malloc(sizeof(VkLayerProperties) * num_layers);
    enabled_layer_names = g_malloc(sizeof(const char*) * num_layers);

    result = vkEnumerateInstanceLayerProperties (&num_layers, layer_props);

    if (result != VK_SUCCESS)
    {
      printf ("Error vkEnumerateInstanceLayerProperties in %d\n", result);
      return false;
    }

    /* OpenVR: no unique_objects when using validation with SteamVR */
    char const *request_layers[] =
    {
      "VK_LAYER_GOOGLE_threading",
      "VK_LAYER_LUNARG_parameter_validation",
      "VK_LAYER_LUNARG_object_tracker",
      "VK_LAYER_LUNARG_image",
      "VK_LAYER_LUNARG_core_validation",
    };

    for (uint32_t i = 0; i < num_layers; i++)
      for (uint32_t j = 0; j < _countof (request_layers); j++)
        if (strstr (layer_props[i].layerName, request_layers[j]) != NULL)
          enabled_layer_names[*num_enabled_layers++] = layer_props[i].layerName;

    *required_extensions = g_slist_append (*required_extensions,
      (gpointer) VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    g_free (layer_props);
  }
  return true;
}

bool
_init_instance_extensions (GSList                *required_extensions,
                           VkExtensionProperties *extension_props,
                           const char           **enabled_extensions,
                           uint32_t              *out_num_enabled_extensions)
{
  uint32_t num_found = 0;
  uint32_t num_extensions = 0;
  VkResult result =
    vkEnumerateInstanceExtensionProperties (NULL, &num_extensions, NULL);
  if (result != VK_SUCCESS)
  {
    g_printerr ("vkEnumerateInstanceExtensionProperties failed with error %d\n",
                result);
    return false;
  }

  extension_props = g_malloc(sizeof(VkExtensionProperties) * num_extensions);

  if (num_extensions > 0)
  {
    result = vkEnumerateInstanceExtensionProperties (NULL, &num_extensions,
                                                     extension_props);
    if (result != VK_SUCCESS)
    {
      g_printerr ("vkEnumerateInstanceExtensionProperties"
                  " failed with error %d\n",
                  result);
      return false;
    }

    for (size_t i = 0; i < g_slist_length (required_extensions); i++)
    {
      bool found = false;
      for (uint32_t j = 0; j < num_extensions; j++)
      {
        GSList* extension_name = g_slist_nth (required_extensions, (guint) i);
        if (strcmp ((gchar*) extension_name->data,
                    extension_props[j].extensionName) == 0)
        {
          found = true;
          enabled_extensions[num_found++] = extension_props[j].extensionName;
          break;
        }
      }

      if (!found)
      {
        GSList* extension_name = g_slist_nth (required_extensions, (guint) i);
        g_printerr ("Vulkan missing requested extension '%s'.\n",
                    (gchar*) extension_name->data);
      }
    }

    if (num_found != (uint32_t) g_slist_length (required_extensions))
      return false;

    *out_num_enabled_extensions = num_found;
  }

  return true;
}

void
_init_validation_callback (OpenVRVulkanUploader *self)
{
  g_pVkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)
    vkGetInstanceProcAddr (self->instance, "vkCreateDebugReportCallbackEXT");
  g_pVkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)
    vkGetInstanceProcAddr (self->instance, "vkDestroyDebugReportCallbackEXT");

  VkDebugReportCallbackCreateInfoEXT debug_report_info = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
    .pNext = NULL,
    .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT,
    .pfnCallback = validation_cb,
    .pUserData = NULL
  };

  g_pVkCreateDebugReportCallbackEXT (self->instance,
                                     &debug_report_info,
                                     NULL, &self->debug_report_cb);
}

bool
openvr_vulkan_uploader_init_vulkan_instance (OpenVRVulkanUploader *self)
{
  GSList* required_extensions = NULL;
  _get_required_instance_extensions (self, &required_extensions);

  uint32_t num_enabled_layers = 0;
  const char **enabled_layers = NULL;

  if (self->enable_validation)
    if (!_init_validation_layers (enabled_layers,
                                 &num_enabled_layers,
                                 &required_extensions))
      return false;

  const char** enabled_extensions =
    g_malloc(sizeof(const char*) * g_slist_length (required_extensions));

  uint32_t num_enabled_extensions = 0;
  VkExtensionProperties *extension_properties = NULL;

  if (!_init_instance_extensions (required_extensions,
                                  extension_properties,
                                  enabled_extensions,
                                 &num_enabled_extensions))
    return false;

  VkResult result =
    vkCreateInstance (&(VkInstanceCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = NULL,
      .pApplicationInfo = &(VkApplicationInfo) {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "vulkan_overlay",
        .applicationVersion = 1,
        .pEngineName = NULL,
        .engineVersion = 1,
        .apiVersion = VK_MAKE_VERSION (1, 1, 70)
      },
      .enabledExtensionCount = num_enabled_extensions,
      .ppEnabledExtensionNames = enabled_extensions,
      .enabledLayerCount = num_enabled_layers,
      .ppEnabledLayerNames = enabled_layers
    },
    NULL,
    &self->instance);

  if (result != VK_SUCCESS)
  {
    g_printerr ("vkCreateInstance failed with error %s\n",
                result_string (result));
    return false;
  }

  if (self->enable_validation)
    _init_validation_callback (self);

  g_free (enabled_extensions);
  g_free (extension_properties);
  g_free (enabled_layers);

  return true;
}

bool
openvr_vulkan_uploader_init_vulkan_device (OpenVRVulkanUploader *self)
{
  uint32_t nDeviceCount = 0;
  VkResult result = vkEnumeratePhysicalDevices (self->instance,
                                                 &nDeviceCount, NULL);
  if (result != VK_SUCCESS || nDeviceCount == 0)
  {
    printf ("vkEnumeratePhysicalDevices failed, "
            "unable to init and enumerate GPUs with Vulkan.\n");
    return false;
  }

  VkPhysicalDevice *pPhysicalDevices =
    g_malloc(sizeof(VkPhysicalDevice) * nDeviceCount);
  result = vkEnumeratePhysicalDevices (self->instance, &nDeviceCount,
                                        pPhysicalDevices);
  if (result != VK_SUCCESS || nDeviceCount == 0)
  {
    printf ("vkEnumeratePhysicalDevices failed, "
            "unable to init and enumerate GPUs with Vulkan.\n");
    return false;
  }

  /* Query OpenVR for the physical device to use */
  uint64_t pHMDPhysicalDevice = 0;
  self->system->GetOutputDevice (&pHMDPhysicalDevice,
                                      ETextureType_TextureType_Vulkan,
                                      (struct VkInstance_T*) self->instance);

  /* Select the HMD physical device */
  self->physical_device = VK_NULL_HANDLE;
  for (uint32_t nPhysicalDevice = 0; nPhysicalDevice < nDeviceCount;
       nPhysicalDevice++)
  {
    if (((VkPhysicalDevice) pHMDPhysicalDevice) ==
      pPhysicalDevices[nPhysicalDevice])
    {
      self->physical_device = (VkPhysicalDevice) pHMDPhysicalDevice;
      break;
    }
  }
  if (self->physical_device == VK_NULL_HANDLE)
  {
    /* Fallback: Grab the first physical device */
    printf ("Failed to find GetOutputDevice VkPhysicalDevice, "
            "falling back to choosing first device.\n");
    self->physical_device = pPhysicalDevices[ 0 ];
  }
  g_free (pPhysicalDevices);

  vkGetPhysicalDeviceProperties (self->physical_device,
                                 &self->physical_device_properties);
  vkGetPhysicalDeviceMemoryProperties (
    self->physical_device,
    &self->physical_device_memory_properties);
  vkGetPhysicalDeviceFeatures (self->physical_device,
                               &self->physical_device_features);

  /* VkDevice creation */
  /* Query OpenVR for the required device extensions for this physical device */
  GSList *requiredDeviceExtensions = NULL;
  _get_required_device_extensions (self,
                                   self->physical_device,
                                   &requiredDeviceExtensions);
  /* Add additional required extensions */
  requiredDeviceExtensions =
    g_slist_append (requiredDeviceExtensions,
                    (gpointer) VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  /* Find the first graphics queue */
  uint32_t nQueueCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties (self->physical_device,
                                            &nQueueCount, 0);
  VkQueueFamilyProperties *pQueueFamilyProperties =
    g_malloc(sizeof(VkQueueFamilyProperties) * nQueueCount);
  vkGetPhysicalDeviceQueueFamilyProperties (self->physical_device,
                                            &nQueueCount,
                                            pQueueFamilyProperties);
  if (nQueueCount == 0)
  {
    printf ("Failed to get queue properties.\n");
    return false;
  }
  uint32_t nGraphicsQueueIndex = 0;
  for (nGraphicsQueueIndex = 0; nGraphicsQueueIndex < nQueueCount;
       nGraphicsQueueIndex++)
  {
    if (pQueueFamilyProperties[nGraphicsQueueIndex].queueFlags &
        VK_QUEUE_GRAPHICS_BIT)
    {
      break;
    }
  }
  if (nGraphicsQueueIndex >= nQueueCount)
  {
    printf ("No graphics queue found\n");
    return false;
  }
  self->queue_family_index = nGraphicsQueueIndex;
  g_free (pQueueFamilyProperties);

  uint32_t nDeviceExtensionCount = 0;
  result = vkEnumerateDeviceExtensionProperties (self->physical_device, NULL,
                                                &nDeviceExtensionCount, NULL);
  if (result != VK_SUCCESS)
  {
    printf ("vkEnumerateDeviceExtensionProperties failed with error %d\n",
            result);
    return false;
  }

  /*
   * Allocate enough ExtensionProperties
   * to support all extensions being enabled
   */
  const char **ppDeviceExtensionNames =
    g_malloc(sizeof(const char *) * nDeviceExtensionCount);
  uint32_t nEnabledDeviceExtensionCount = 0;

  /* Enable required device extensions */
  VkExtensionProperties *pDeviceExtProperties =
    g_malloc(sizeof(VkExtensionProperties) * nDeviceExtensionCount);
  memset (pDeviceExtProperties, 0,
          sizeof (VkExtensionProperties) * nDeviceExtensionCount);
  if (nDeviceExtensionCount > 0)
  {
    result = vkEnumerateDeviceExtensionProperties (self->physical_device,
                                                   NULL,
                                                  &nDeviceExtensionCount,
                                                   pDeviceExtProperties);
    if (result != VK_SUCCESS)
    {
      printf ("vkEnumerateDeviceExtensionProperties failed with error %d\n",
              result);
      return false;
    }

    for (size_t nRequiredDeviceExt = 0;
         nRequiredDeviceExt < g_slist_length (requiredDeviceExtensions);
         nRequiredDeviceExt++)
    {
      bool bExtFound = false;
      for (uint32_t nDeviceExt = 0; nDeviceExt < nDeviceExtensionCount;
           nDeviceExt++)
      {
        GSList* extension_name = g_slist_nth (requiredDeviceExtensions,
                                              (guint) nRequiredDeviceExt);
        if (strcmp ((gchar*)extension_name->data,
                    pDeviceExtProperties[ nDeviceExt ].extensionName) == 0)
        {
          bExtFound = true;
          break;
        }
      }

      if (bExtFound)
      {
        GSList* extension_name = g_slist_nth (requiredDeviceExtensions,
                                              (guint) nRequiredDeviceExt);
        ppDeviceExtensionNames[nEnabledDeviceExtensionCount] =
          (gchar*)extension_name->data;
        nEnabledDeviceExtensionCount++;
      }
    }
  }

  /* Create the device */
  float queue_priority = 1.0f;
  VkDeviceQueueCreateInfo device_queue_info =
  {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = self->queue_family_index,
    .queueCount = 1,
    .pQueuePriorities = &queue_priority
  };

  VkDeviceCreateInfo device_info =
  {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &device_queue_info,
    .enabledExtensionCount = nEnabledDeviceExtensionCount,
    .ppEnabledExtensionNames = ppDeviceExtensionNames,
    .pEnabledFeatures = &self->physical_device_features
  };

  result = vkCreateDevice (self->physical_device,
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
 * Initialize Vulkan. Returns true if Vulkan has been successfully initialized,
 * false if shaders could not be created.
 * If failure occurred in a module other than shaders,
 * the function may return true or throw an error.
 */
bool
openvr_vulkan_uploader_init_vulkan (OpenVRVulkanUploader *self)
{
  if (!openvr_vulkan_uploader_init_vulkan_instance (self))
    return false;

  if (!openvr_vulkan_uploader_init_vulkan_device (self))
    return false;

  /* Create the command pool */
  {
    VkCommandPoolCreateInfo command_pool_info =
    {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = self->queue_family_index,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    };

    VkResult result = vkCreateCommandPool (self->device, &command_pool_info,
                                           NULL, &self->command_pool);
    if (result != VK_SUCCESS)
    {
      g_printerr ("vkCreateCommandPool returned error %d.", result);
      return false;
    }
  }

  /* Command buffer used during resource loading */
  self->current_cmd_buffer =
        openvr_vulkan_uploader_get_command_buffer (self);
  VkCommandBufferBeginInfo command_buffer_begin_info =
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  };
  vkBeginCommandBuffer (self->current_cmd_buffer->cmd_buffer,
                        &command_buffer_begin_info);

  openvr_vulkan_uploader_init_texture (self);

  /* Submit the command buffer used during loading */
  vkEndCommandBuffer (self->current_cmd_buffer->cmd_buffer);

  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &self->current_cmd_buffer->cmd_buffer
  };

  vkQueueSubmit (self->queue, 1, &submit_info, self->current_cmd_buffer->fence);
  g_queue_push_head (self->cmd_buffers, (gpointer) self->current_cmd_buffer);

  /* Wait for the GPU before proceeding */
  vkQueueWaitIdle (self->queue);

  return true;
}

/*
 * Initialize Compositor.
 * Returns true if the compositor was successfully initialized, false otherwise.
 */
bool
openvr_vulkan_uploader_init_compositor (OpenVRVulkanUploader *self)
{
  if (!_compositor_init_fn_table (self))
  {
    g_printerr ("Compositor initialization failed.\n");
    return false;
  }
  return true;
}

void
_cleanup_command_buffer_queue (gpointer item, OpenVRVulkanUploader *self)
{
  VulkanCommandBuffer_t *b = (VulkanCommandBuffer_t*) item;
  vkFreeCommandBuffers (self->device, self->command_pool, 1, &b->cmd_buffer);
  vkDestroyFence (self->device, b->fence, NULL);
  // g_object_unref (item);
}

void
openvr_vulkan_uploader_shutdown (OpenVRVulkanUploader *self)
{
  /* Idle the device to make sure no work is outstanding */
  if (self->device != VK_NULL_HANDLE)
    vkDeviceWaitIdle (self->device);

  if (self->system != NULL)
  {
    VR_ShutdownInternal();
    self->system = NULL;
  }

  if (self->device != VK_NULL_HANDLE)
  {
    g_queue_foreach (self->cmd_buffers,
                     (GFunc) _cleanup_command_buffer_queue, self);

    vkDestroyCommandPool (self->device, self->command_pool, NULL);

    vkDestroyImageView (self->device, self->image_view, NULL);
    vkDestroyImage (self->device, self->image, NULL);
    vkFreeMemory (self->device, self->image_memory, NULL);
    vkDestroyBuffer (self->device, self->staging_buffer, NULL);
    vkFreeMemory (self->device, self->staging_buffer_memory, NULL);

    if (self->debug_report_cb != VK_NULL_HANDLE)
      g_pVkDestroyDebugReportCallbackEXT (self->instance,
                                          self->debug_report_cb, NULL);

    vkDestroyDevice (self->device, NULL);
    vkDestroyInstance (self->instance, NULL);
  }
}

void
openvr_vulkan_uploader_submit_frame (OpenVRVulkanUploader *self)
{
  /* Submit to SteamVR */
  struct VRVulkanTextureData_t texture_data =
    {
      .m_nImage = (uint64_t) self->image,
      .m_pDevice = (struct VkDevice_T*) self->device,
      .m_pPhysicalDevice = (struct VkPhysicalDevice_T*) self->physical_device,
      .m_pInstance = (struct VkInstance_T*) self->instance,
      .m_pQueue = (struct VkQueue_T*) self->queue,
      .m_nQueueFamilyIndex = self->queue_family_index,
      .m_nWidth = self->width,
      .m_nHeight = self->height,
      .m_nFormat = VK_FORMAT_R8G8B8A8_SRGB,
      .m_nSampleCount = 1
    };

  struct Texture_t texture =
    {
      .handle = &texture_data,
      .eType = ETextureType_TextureType_Vulkan,
      .eColorSpace = EColorSpace_ColorSpace_Auto
    };

  self->overlay->SetOverlayTexture (self->overlay_handle, &texture);
}

bool
openvr_vulkan_uploader_init_texture (OpenVRVulkanUploader *self)
{
  GError *error = NULL;
  GdkPixbuf * pixbuf = gdk_pixbuf_new_from_resource ("/res/cat.jpg", &error);

  if (error != NULL)
    {
      g_printerr ("Could not read texture file.\n");
      return false;
    }

  self->width = (guint) gdk_pixbuf_get_width (pixbuf);
  self->height = (guint) gdk_pixbuf_get_height (pixbuf);

  GdkPixbuf *pixbuf_with_alpha = gdk_pixbuf_add_alpha (pixbuf, false, 0, 0, 0);
  g_object_unref (pixbuf);

  guchar *pixels = gdk_pixbuf_get_pixels (pixbuf_with_alpha);

  gsize pixbuf_alpha_size = gdk_pixbuf_get_byte_length (pixbuf_with_alpha);

  VkBufferImageCopy buffer_image_copy = {
    .bufferOffset = 0,
    .bufferRowLength = 0,
    .bufferImageHeight = 0,
    .imageSubresource = {
      .baseArrayLayer = 0,
      .layerCount = 1,
      .mipLevel = 0,
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    },
    .imageOffset = {
      .x = 0,
      .y = 0,
      .z = 0,
     },
    .imageExtent = {
      .width = self->width,
      .height = self->height,
      .depth = 1,
    }
  };

  /* Create the image */
  VkImageCreateInfo image_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .extent.width = self->width,
    .extent.height = self->height,
    .extent.depth = 1,
    .mipLevels = 1,
    .arrayLayers = 1,
    .format = VK_FORMAT_R8G8B8A8_UNORM,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .usage = VK_IMAGE_USAGE_SAMPLED_BIT |
             VK_IMAGE_USAGE_TRANSFER_DST_BIT |
             VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    .flags = 0
  };
  vkCreateImage (self->device, &image_info, NULL, &self->image);

  VkMemoryRequirements memory_requirements = {};
  vkGetImageMemoryRequirements (self->device, self->image,
                                &memory_requirements);

  VkMemoryAllocateInfo memory_info =
  {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memory_requirements.size
  };
  memory_type_from_properties (&self->physical_device_memory_properties,
                               memory_requirements.memoryTypeBits,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               &memory_info.memoryTypeIndex);
  vkAllocateMemory (self->device, &memory_info, NULL, &self->image_memory);
  vkBindImageMemory (self->device, self->image, self->image_memory, 0);

  VkImageViewCreateInfo image_view_info =
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .flags = 0,
    .image = self->image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = image_info.format,
    .components = {
      .r = VK_COMPONENT_SWIZZLE_IDENTITY,
      .g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    },
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = image_info.mipLevels,
      .baseArrayLayer = 0,
      .layerCount = 1,
    }
  };
  vkCreateImageView (self->device, &image_view_info, NULL, &self->image_view);

  /* Create a staging buffer */
  if (!create_vulkan_buffer (self->device,
                             &self->physical_device_memory_properties,
                             pixels,
                             pixbuf_alpha_size,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             &self->staging_buffer,
                             &self->staging_buffer_memory))
    return false;

  /* Transition the image to TRANSFER_DST to receive image */
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
      .levelCount = image_info.mipLevels,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
    .srcQueueFamilyIndex = self->queue_family_index,
    .dstQueueFamilyIndex = self->queue_family_index
  };

  vkCmdPipelineBarrier (self->current_cmd_buffer->cmd_buffer,
                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                        &image_memory_barrier);

  /* Issue the copy to fill the image data */
  vkCmdCopyBufferToImage (self->current_cmd_buffer->cmd_buffer,
                          self->staging_buffer, self->image,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          1,
                          &buffer_image_copy);

  /* Transition the image to SHADER_READ_OPTIMAL for reading */
  image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  vkCmdPipelineBarrier (self->current_cmd_buffer->cmd_buffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0,
                        NULL, 1, &image_memory_barrier);

  g_object_unref (pixbuf_with_alpha);
  return true;
}

/*
 * Get an available command buffer or create a new one if one available.
 * Associate a fence with the command buffer.
 */
VulkanCommandBuffer_t*
openvr_vulkan_uploader_get_command_buffer (OpenVRVulkanUploader *self)
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

    if (vkGetFenceStatus (self->device, tail->fence) == VK_SUCCESS)
    {
      command_buffer->cmd_buffer = tail->cmd_buffer;
      command_buffer->fence = tail->fence;

      vkResetCommandBuffer (command_buffer->cmd_buffer,
                            VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
      vkResetFences (self->device, 1, &command_buffer->fence);
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
  vkAllocateCommandBuffers (self->device, &command_buffer_info,
                            &command_buffer->cmd_buffer);

  VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
  };
  vkCreateFence (self->device, &fence_info, NULL, &command_buffer->fence);
  return command_buffer;
}
