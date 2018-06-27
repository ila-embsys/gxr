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

#include "openvr-global.h"

G_DEFINE_TYPE (OpenVRCompositor, openvr_compositor, G_TYPE_OBJECT)

gboolean
_compositor_init_fn_table (OpenVRCompositor *self)
{
  INIT_FN_TABLE (self->functions, Compositor);
}

static void
openvr_compositor_init (OpenVRCompositor *self)
{
  self->functions = NULL;
  if (!_compositor_init_fn_table (self))
    g_printerr ("Compositor functions failed to load.\n");

  if (!self->functions)
    g_printerr ("Compositor functions failed to load.\n");
}

OpenVRCompositor *
openvr_compositor_new (void)
{
  return (OpenVRCompositor*) g_object_new (OPENVR_TYPE_COMPOSITOR, 0);
}

static void
openvr_compositor_finalize (GObject *gobject)
{
  // OpenVRCompositor *self = OPENVR_COMPOSITOR (gobject);
}

static void
openvr_compositor_class_init (OpenVRCompositorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_compositor_finalize;
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
openvr_compositor_get_instance_extensions (OpenVRCompositor *self,
                                           GSList          **out_list)
{
  uint32_t size =
    self->functions->GetVulkanInstanceExtensionsRequired (NULL, 0);
  if (size > 0)
  {
    gchar *all_extensions = g_malloc(sizeof(gchar) * size);
    all_extensions[0] = 0;
    self->functions->GetVulkanInstanceExtensionsRequired (all_extensions, size);
    _split (all_extensions, out_list);
    g_free(all_extensions);
  }
}

/* Ask OpenVR for the list of device extensions required */
void
openvr_compositor_get_device_extensions (OpenVRCompositor *self,
                                         VkPhysicalDevice  physical_device,
                                         GSList          **out_list)
{
  uint32_t size = self->functions->GetVulkanDeviceExtensionsRequired (
    (struct VkPhysicalDevice_T*) physical_device, NULL, 0);

  if (size > 0)
  {
    gchar *all_extensions = g_malloc(sizeof(gchar) * size);
    all_extensions[0] = 0;
    self->functions->GetVulkanDeviceExtensionsRequired (
      (struct VkPhysicalDevice_T*) physical_device, all_extensions, size);

    _split (all_extensions, out_list);
    g_free (all_extensions);
  }
}
