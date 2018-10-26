/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_COMPOSITOR_H_
#define OPENVR_GLIB_COMPOSITOR_H_

#include <glib-object.h>
#include <openvr_capi.h>

#include <vulkan/vulkan.h>

G_BEGIN_DECLS

void
openvr_compositor_get_instance_extensions (GSList **out_list);

void
openvr_compositor_get_device_extensions (VkPhysicalDevice  physical_device,
                                         GSList          **out_list);

G_END_DECLS

#endif /* OPENVR_GLIB_COMPOSITOR_H_ */
