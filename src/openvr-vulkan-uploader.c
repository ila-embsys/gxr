/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-uploader.h"
#include "openvr-context.h"

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#include <gmodule.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "openvr-overlay.h"
#include "openvr-compositor.h"


G_DEFINE_TYPE (OpenVRVulkanUploader, openvr_vulkan_uploader,
               OPENVR_TYPE_VULKAN_CLIENT)

static void
openvr_vulkan_uploader_finalize (GObject *gobject);

static void
openvr_vulkan_uploader_class_init (OpenVRVulkanUploaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_vulkan_uploader_finalize;
}

static void
openvr_vulkan_uploader_init (OpenVRVulkanUploader *self)
{
  (void) self;
}

OpenVRVulkanUploader *
openvr_vulkan_uploader_new (void)
{
  return (OpenVRVulkanUploader*) g_object_new (OPENVR_TYPE_VULKAN_UPLOADER, 0);
}

static void
openvr_vulkan_uploader_finalize (GObject *gobject)
{
  OpenVRVulkanClient *client = OPENVR_VULKAN_CLIENT (gobject);

  /* Idle the device to make sure no work is outstanding */
  if (client->device->device != VK_NULL_HANDLE)
    vkDeviceWaitIdle (client->device->device);

  G_OBJECT_CLASS (openvr_vulkan_uploader_parent_class)->finalize (gobject);
}

bool
openvr_vulkan_uploader_init_vulkan (OpenVRVulkanUploader *self,
                                    bool enable_validation)
{
  OpenVRVulkanClient *client = OPENVR_VULKAN_CLIENT (self);
  GSList* openvr_instance_extensions = NULL;
  openvr_compositor_get_instance_extensions (&openvr_instance_extensions);

  if (!openvr_vulkan_instance_create (client->instance,
                                      enable_validation,
                                      openvr_instance_extensions))
    {
      g_printerr ("Failed to create instance.\n");
      return false;
    }

  /* Query OpenVR for the physical device to use */
  uint64_t physical_device = 0;
  OpenVRContext *context = openvr_context_get_instance ();
  context->system->GetOutputDevice (
    &physical_device, ETextureType_TextureType_Vulkan,
    (struct VkInstance_T*) client->instance->instance);

  g_print ("OpenVR requests device %ld.\n", physical_device);

  /* Query required OpenVR device extensions */
  GSList *openvr_device_extensions = NULL;
  openvr_compositor_get_device_extensions ((VkPhysicalDevice) physical_device,
                                          &openvr_device_extensions);

  if (!openvr_vulkan_device_create (client->device,
                                    client->instance,
                                    (VkPhysicalDevice) physical_device,
                                    openvr_device_extensions))
    {
      g_printerr ("Failed to create device.\n");
      return false;
    }

  if (!openvr_vulkan_client_init_command_pool (client))
    {
      g_printerr ("Failed to create command pool.\n");
      return false;
    }

  return true;
}

/* Submit frame to OpenVR runtime */
void
openvr_vulkan_uploader_submit_frame (OpenVRVulkanUploader *self,
                                     OpenVROverlay        *overlay,
                                     OpenVRVulkanTexture  *texture)
{
  OpenVRVulkanClient *client = OPENVR_VULKAN_CLIENT (self);
  OpenVRVulkanDevice *device = client->device;

  struct VRVulkanTextureData_t texture_data =
    {
      .m_nImage = (uint64_t) texture->image,
      .m_pDevice = (struct VkDevice_T*) device->device,
      .m_pPhysicalDevice = (struct VkPhysicalDevice_T*) device->physical_device,
      .m_pInstance = (struct VkInstance_T*) client->instance->instance,
      .m_pQueue = (struct VkQueue_T*) device->queue,
      .m_nQueueFamilyIndex = device->queue_family_index,
      .m_nWidth = texture->width,
      .m_nHeight = texture->height,
      .m_nFormat = VK_FORMAT_B8G8R8A8_UNORM,
      .m_nSampleCount = 1
    };

  struct Texture_t vr_texture =
    {
      .handle = &texture_data,
      .eType = ETextureType_TextureType_Vulkan,
      .eColorSpace = EColorSpace_ColorSpace_Auto
    };

  OpenVRContext *context = openvr_context_get_instance ();
  context->overlay->SetOverlayTexture (overlay->overlay_handle, &vr_texture);
}
