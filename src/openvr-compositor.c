/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib/gprintf.h>
#include <openvr_capi.h>
#include "openvr_capi_global.h"

#include "openvr-compositor.h"
#include "openvr-context.h"

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
openvr_compositor_get_instance_extensions (GSList **out_list)
{
  OpenVRContext *context = openvr_context_get_instance ();
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
}

/* Ask OpenVR for the list of device extensions required */
void
openvr_compositor_get_device_extensions (VkPhysicalDevice  physical_device,
                                         GSList          **out_list)
{
  OpenVRContext *context = openvr_context_get_instance ();
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
}

GulkanClient*
openvr_compositor_gulkan_client_new (bool enable_validation)
{
  GulkanClient *client = gulkan_client_new ();
  GulkanDevice *device = gulkan_client_get_device (client);
  GulkanInstance *instance = gulkan_client_get_instance (client);

  GSList* openvr_instance_extensions = NULL;
  openvr_compositor_get_instance_extensions (&openvr_instance_extensions);

  if (!gulkan_instance_create (instance,
                               enable_validation,
                               openvr_instance_extensions))
    {
      g_printerr ("Failed to create instance.\n");
      g_object_unref (client);
      return NULL;
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

  if (!gulkan_device_create (device,
                             instance,
                             (VkPhysicalDevice) physical_device,
                             openvr_device_extensions))
    {
      g_printerr ("Failed to create device.\n");
      g_object_unref (client);
      return NULL;
    }

  if (!gulkan_client_init_command_pool (client))
    {
      g_printerr ("Failed to create command pool.\n");
      g_object_unref (client);
      return NULL;
    }

  return client;
}
