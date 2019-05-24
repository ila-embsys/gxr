/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-overlay-uploader.h"
#include "openvr-context.h"

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#include <gmodule.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "openvr-overlay.h"
#include "openvr-compositor.h"

struct _GulkanClient
{
  GulkanClient parent_type;
};

G_DEFINE_TYPE (GulkanClient, openvr_overlay_uploader,
               GULKAN_TYPE_CLIENT)

static void
openvr_overlay_uploader_finalize (GObject *gobject);

static void
openvr_overlay_uploader_class_init (GulkanClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_overlay_uploader_finalize;
}

static void
openvr_overlay_uploader_init (GulkanClient *self)
{
  (void) self;
}

GulkanClient *
openvr_overlay_uploader_new (void)
{
  return (GulkanClient*) g_object_new (OPENVR_TYPE_OVERLAY_UPLOADER, 0);
}

static void
openvr_overlay_uploader_finalize (GObject *gobject)
{
  GulkanClient *client = GULKAN_CLIENT (gobject);

  gulkan_device_wait_idle (gulkan_client_get_device (client));

  G_OBJECT_CLASS (openvr_overlay_uploader_parent_class)->finalize (gobject);
}

bool
openvr_overlay_uploader_init_vulkan (GulkanClient *self,
                                    bool enable_validation)
{
  GulkanClient *client = GULKAN_CLIENT (self);
  GulkanDevice *device = gulkan_client_get_device (client);
  GulkanInstance *instance = gulkan_client_get_instance (client);

  GSList* openvr_instance_extensions = NULL;
  openvr_compositor_get_instance_extensions (&openvr_instance_extensions);

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

  g_print ("OpenVR requests device %ld.\n", physical_device);

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
      return false;
    }

  if (!gulkan_client_init_command_pool (client))
    {
      g_printerr ("Failed to create command pool.\n");
      return false;
    }

  return true;
}


