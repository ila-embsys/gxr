/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_VULKAN_CLIENT_H_
#define OPENVR_GLIB_VULKAN_CLIENT_H_

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#include "openvr-vulkan-instance.h"
#include "openvr-vulkan-device.h"
#include "openvr-vulkan-texture.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_VULKAN_CLIENT            (openvr_vulkan_client_get_type())
#define OPENVR_VULKAN_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OPENVR_TYPE_VULKAN_CLIENT, OpenVRVulkanClient))
#define OPENVR_VULKAN_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OPENVR_TYPE_VULKAN_CLIENT, OpenVRVulkanClientClass))
#define OPENVR_IS_VULKAN_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OPENVR_TYPE_VULKAN_CLIENT))
#define OPENVR_IS_VULKAN_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OPENVR_TYPE_VULKAN_CLIENT))
#define OPENVR_VULKAN_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), OPENVR_TYPE_VULKAN_CLIENT, OpenVRVulkanClientClass))

typedef struct _OpenVRVulkanClient             OpenVRVulkanClient;
typedef struct _OpenVRVulkanClientClass        OpenVRVulkanClientClass;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(OpenVRVulkanClient, g_object_unref)

typedef struct
{
  VkCommandBuffer cmd_buffer;
  VkFence fence;
} FencedCommandBuffer;

struct _OpenVRVulkanClient
{
  GObject object;

  VkCommandPool command_pool;

  OpenVRVulkanInstance *instance;
  OpenVRVulkanDevice *device;
};

struct _OpenVRVulkanClientClass
{
  GObjectClass parent_class;
};

GType      openvr_vulkan_client_get_type                 (void) G_GNUC_CONST;

OpenVRVulkanClient * openvr_vulkan_client_new (void);

bool
openvr_vulkan_client_begin_res_cmd_buffer (OpenVRVulkanClient  *self,
                                           FencedCommandBuffer *buffer);

bool
openvr_vulkan_client_submit_res_cmd_buffer (OpenVRVulkanClient  *self,
                                            FencedCommandBuffer *buffer);

bool
openvr_vulkan_client_upload_pixels (OpenVRVulkanClient   *self,
                                    OpenVRVulkanTexture  *texture,
                                    guchar               *pixels,
                                    gsize                 size);

bool
openvr_vulkan_client_upload_pixbuf (OpenVRVulkanClient   *self,
                                    OpenVRVulkanTexture  *texture,
                                    GdkPixbuf            *pixbuf);

bool
openvr_vulkan_client_upload_cairo_surface (OpenVRVulkanClient  *self,
                                           OpenVRVulkanTexture *texture,
                                           cairo_surface_t     *surface);

bool
openvr_vulkan_client_init_vulkan (OpenVRVulkanClient *self,
                                  GSList             *instance_extensions,
                                  GSList             *device_extensions,
                                  bool                enable_validation);

bool
openvr_vulkan_client_transfer_layout (OpenVRVulkanClient  *self,
                                      OpenVRVulkanTexture *texture,
                                      VkImageLayout        old_layout,
                                      VkImageLayout        new_layout);

bool
openvr_vulkan_client_init_command_pool (OpenVRVulkanClient *self);

G_END_DECLS

#endif /* OPENVR_GLIB_VULKAN_CLIENT_H_ */
