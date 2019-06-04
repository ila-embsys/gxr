/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_COMPOSITOR_H_
#define OPENVR_GLIB_COMPOSITOR_H_

#if !defined (OPENVR_GLIB_INSIDE) && !defined (OPENVR_GLIB_COMPILATION)
#error "Only <openvr-glib.h> can be included directly."
#endif

#include <glib-object.h>
#include <openvr_capi.h>

#include <vulkan/vulkan.h>
#include <gulkan.h>

G_BEGIN_DECLS

bool
openvr_compositor_get_instance_extensions (GSList **out_list);

bool
openvr_compositor_get_device_extensions (VkPhysicalDevice  physical_device,
                                         GSList          **out_list);

bool
openvr_compositor_gulkan_client_init (GulkanClient *client,
                                      bool          enable_validation);

GulkanClient*
openvr_compositor_gulkan_client_new (bool enable_validation);

bool
openvr_compositor_submit (GulkanClient         *client,
                          uint32_t              width,
                          uint32_t              height,
                          VkFormat              format,
                          VkSampleCountFlagBits sample_count,
                          VkImage               left,
                          VkImage               right);

G_END_DECLS

#endif /* OPENVR_GLIB_COMPOSITOR_H_ */
