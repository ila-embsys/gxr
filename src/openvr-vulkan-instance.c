/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-instance.h"

#include <vulkan/vulkan.h>
#include <stdbool.h>

#ifndef _countof
#define _countof(x) (sizeof (x) / sizeof ((x)[0]))
#endif

/* OpenVR: no unique_objects when using validation with SteamVR */
static gchar const *validation_layers[] =
  {
    "VK_LAYER_GOOGLE_threading",
    "VK_LAYER_LUNARG_parameter_validation",
    "VK_LAYER_LUNARG_object_tracker",
    "VK_LAYER_LUNARG_image",
    "VK_LAYER_LUNARG_core_validation",
  };

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


G_DEFINE_TYPE (OpenVRVulkanInstance, openvr_vulkan_instance, G_TYPE_OBJECT)

/* Vulkan extension entrypoints */
static PFN_vkCreateDebugReportCallbackEXT
  g_pVkCreateDebugReportCallbackEXT = NULL;
static PFN_vkDestroyDebugReportCallbackEXT
  g_pVkDestroyDebugReportCallbackEXT = NULL;

static void
openvr_vulkan_instance_init (OpenVRVulkanInstance *self)
{
  self->instance = VK_NULL_HANDLE;
  self->debug_report_cb = VK_NULL_HANDLE;
}

OpenVRVulkanInstance *
openvr_vulkan_instance_new (void)
{
  return (OpenVRVulkanInstance*) g_object_new (OPENVR_TYPE_VULKAN_INSTANCE, 0);
}

static void
openvr_vulkan_instance_finalize (GObject *gobject)
{
  OpenVRVulkanInstance *self = OPENVR_VULKAN_INSTANCE (gobject);
  if (self->debug_report_cb != VK_NULL_HANDLE)
    g_pVkDestroyDebugReportCallbackEXT (self->instance,
                                        self->debug_report_cb, NULL);

  vkDestroyInstance (self->instance, NULL);
}

static void
openvr_vulkan_instance_class_init (OpenVRVulkanInstanceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_vulkan_instance_finalize;
}

static VkBool32 VKAPI_PTR
validation_cb (VkDebugReportFlagsEXT      flags,
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

bool
_init_validation_layers (uint32_t     num_layers,
                         const char **enabled_layers,
                         uint32_t    *out_num_enabled)
{
  uint32_t num_enabled = 0;

  VkLayerProperties * layer_props =
    g_malloc(sizeof(VkLayerProperties) * num_layers);

  VkResult result =
    vkEnumerateInstanceLayerProperties (&num_layers, layer_props);

  if (result != VK_SUCCESS)
    {
      g_printerr ("Error vkEnumerateInstanceLayerProperties in %d\n", result);
      return false;
    }

  for (uint32_t i = 0; i < num_layers; i++)
    for (uint32_t j = 0; j < _countof (validation_layers); j++)
      if (strstr (layer_props[i].layerName, validation_layers[j]) != NULL)
        enabled_layers[num_enabled++] = g_strdup (layer_props[i].layerName);

  *out_num_enabled = num_enabled;

  g_free (layer_props);

  return true;
}

bool
_init_instance_extensions (GSList      *required_extensions,
                           const char **enabled_extensions,
                           uint32_t    *out_num_enabled)
{
  uint32_t num_enabled = 0;
  uint32_t num_extensions = 0;
  VkResult result =
    vkEnumerateInstanceExtensionProperties (NULL, &num_extensions, NULL);
  if (result != VK_SUCCESS)
  {
    g_printerr ("vkEnumerateInstanceExtensionProperties failed with error %d\n",
                result);
    return false;
  }

  VkExtensionProperties *extension_props =
    g_malloc (sizeof(VkExtensionProperties) * num_extensions);

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
          enabled_extensions[num_enabled++] =
            g_strdup (extension_props[j].extensionName);
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

    if (num_enabled != (uint32_t) g_slist_length (required_extensions))
      return false;

    *out_num_enabled = num_enabled;
  }

  g_free (extension_props);

  return true;
}

void
_init_validation_callback (OpenVRVulkanInstance *self)
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
openvr_vulkan_instance_create (OpenVRVulkanInstance *self,
                               bool enable_validation,
                               OpenVRCompositor *compositor)
{
  GSList* required_extensions = NULL;
  openvr_compositor_get_instance_extensions (compositor, &required_extensions);

  uint32_t num_enabled_layers = 0;
  const char **enabled_layers = NULL;

  if (enable_validation)
    {
      uint32_t num_layers = 0;
      VkResult result = vkEnumerateInstanceLayerProperties (&num_layers, NULL);
      if (result != VK_SUCCESS)
        {
          g_printerr ("vkEnumerateInstanceLayerProperties"
                      " failed with error %d\n",
                      result);
          return false;
        }

      enabled_layers = g_malloc(sizeof(const char*) * num_layers);
      if (num_layers > 0)
        {
          if (!_init_validation_layers (num_layers,
                                        enabled_layers,
                                       &num_enabled_layers))
              return false;
          required_extensions = g_slist_append (required_extensions,
            (gpointer) VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }
    }

  const char** enabled_extensions =
    g_malloc(sizeof(const char*) * g_slist_length (required_extensions));

  uint32_t num_enabled_extensions = 0;

  if (!_init_instance_extensions (required_extensions,
                                  enabled_extensions,
                                 &num_enabled_extensions))
    return false;

  g_print ("Requesting instance extensions:\n");
  for (int i = 0; i < num_enabled_extensions; i++)
      g_print ("%s\n", enabled_extensions[i]);

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

  if (enable_validation)
    _init_validation_callback (self);

  g_free (enabled_extensions);
  g_free (enabled_layers);

  return true;
}

