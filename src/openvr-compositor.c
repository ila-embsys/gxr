/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib/gprintf.h>

#include "openvr-wrapper.h"

#include "openvr-compositor.h"
#include "openvr-context.h"

static void
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
bool
openvr_compositor_get_instance_extensions (GSList **out_list)
{
  OpenVRContext *context = openvr_context_get_instance ();
  if (!openvr_context_is_valid (context))
    {
      g_printerr ("OpenVR context was not initialized.\n");
      return FALSE;
    }

  uint32_t size =
    context->compositor->GetVulkanInstanceExtensionsRequired (NULL, 0);

  if (size > 0)
    {
      gchar *extensions = g_malloc(sizeof(gchar) * size);
      extensions[0] = 0;
      context->compositor->GetVulkanInstanceExtensionsRequired (extensions, size);
      _split (extensions, out_list);
      g_free(extensions);
    }

  return TRUE;
}

/* Ask OpenVR for the list of device extensions required */
bool
openvr_compositor_get_device_extensions (VkPhysicalDevice  physical_device,
                                         GSList          **out_list)
{
  OpenVRContext *context = openvr_context_get_instance ();
  if (!openvr_context_is_valid (context))
    {
      g_printerr ("OpenVR context was not initialized.\n");
      return FALSE;
    }

  uint32_t size = context->compositor->
    GetVulkanDeviceExtensionsRequired (physical_device, NULL, 0);

  if (size > 0)
    {
      gchar *extensions = g_malloc(sizeof(gchar) * size);
      extensions[0] = 0;
      context->compositor->GetVulkanDeviceExtensionsRequired (
        physical_device, extensions, size);

      _split (extensions, out_list);
      g_free (extensions);
    }

  return TRUE;
}

bool
openvr_compositor_gulkan_client_init (GulkanClient *client,
                                      bool          enable_validation)
{
  GSList* openvr_instance_extensions = NULL;
  if (!openvr_compositor_get_instance_extensions (&openvr_instance_extensions))
    return FALSE;

  GulkanInstance *instance = gulkan_client_get_instance (client);
  if (!gulkan_instance_create (instance,
                               enable_validation,
                               openvr_instance_extensions))
    {
      g_printerr ("Failed to create instance.\n");
      return false;
    }

  VkInstance instance_handle = gulkan_instance_get_handle (instance);

  /* Query OpenVR for the physical device to use */
  uint64_t physical_device = 0;
  OpenVRContext *context = openvr_context_get_instance ();
  context->system->GetOutputDevice (
    &physical_device, ETextureType_TextureType_Vulkan,
    (struct VkInstance_T*) instance_handle);

  /* Query required OpenVR device extensions */
  GSList *openvr_device_extensions = NULL;
  openvr_compositor_get_device_extensions ((VkPhysicalDevice) physical_device,
                                          &openvr_device_extensions);

  GulkanDevice *device = gulkan_client_get_device (client);
  if (!gulkan_device_create (device,
                             instance,
                             (VkPhysicalDevice) physical_device,
                             openvr_device_extensions))
    {
      g_printerr ("Failed to create device.\n");
      return false;
    }

  if (!gulkan_client_init_command_pool (client))
    {
      g_printerr ("Failed to create command pool.\n");
      return false;
    }

  return true;
}

GulkanClient*
openvr_compositor_gulkan_client_new (bool enable_validation)
{
  GulkanClient *client = gulkan_client_new ();
  if (!openvr_compositor_gulkan_client_init (client, enable_validation))
    {
      g_object_unref (client);
      return NULL;
    }
  return client;
}

#define ENUM_TO_STR(r) case r: return #r

static const gchar*
_compositor_error_to_str (EVRCompositorError err)
{
  switch (err)
    {
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_None);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_RequestFailed);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_IncompatibleVersion);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_DoNotHaveFocus);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_InvalidTexture);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_IsNotSceneApplication);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_TextureIsOnWrongDevice);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_TextureUsesUnsupportedFormat);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_SharedTexturesNotSupported);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_IndexOutOfRange);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_AlreadySubmitted);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_InvalidBounds);
      default:
        return "Unknown compositor error.";
    }
}

bool
openvr_compositor_submit (GulkanClient         *client,
                          uint32_t              width,
                          uint32_t              height,
                          VkFormat              format,
                          VkSampleCountFlagBits sample_count,
                          VkImage               left,
                          VkImage               right)
{
  GulkanDevice *device = gulkan_client_get_device (client);
  VkDevice device_handle = gulkan_device_get_handle (device);

  VRVulkanTextureData_t openvr_texture_data = {
    .m_nImage = (uint64_t) left,
    .m_pDevice = device_handle,
    .m_pPhysicalDevice = gulkan_client_get_physical_device_handle (client),
    .m_pInstance = gulkan_client_get_instance_handle (client),
    .m_pQueue = gulkan_device_get_queue_handle (device),
    .m_nQueueFamilyIndex = gulkan_device_get_queue_family_index (device),
    .m_nWidth = width,
    .m_nHeight = height,
    .m_nFormat = format,
    .m_nSampleCount = sample_count
  };

  Texture_t texture = {
    &openvr_texture_data,
    ETextureType_TextureType_Vulkan,
    EColorSpace_ColorSpace_Auto
  };

  VRTextureBounds_t bounds = {
    .uMin = 0.0f,
    .uMax = 1.0f,
    .vMin = 0.0f,
    .vMax = 1.0f
  };

  OpenVRContext *context = openvr_context_get_instance ();
  EVRCompositorError err =
    context->compositor->Submit (EVREye_Eye_Left, &texture, &bounds,
                                 EVRSubmitFlags_Submit_Default);

  if (err != EVRCompositorError_VRCompositorError_None)
    {
      g_printerr ("Submit returned error: %s\n",
                  _compositor_error_to_str (err));
      return false;
    }

  openvr_texture_data.m_nImage = (uint64_t) right;
  err =
    context->compositor->Submit (EVREye_Eye_Right, &texture, &bounds,
                                 EVRSubmitFlags_Submit_Default);

  if (err != EVRCompositorError_VRCompositorError_None)
    {
      g_printerr ("Submit returned error: %s\n",
                  _compositor_error_to_str (err));
      return false;
    }

  return true;
}

