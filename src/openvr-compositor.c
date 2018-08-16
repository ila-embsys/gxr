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
