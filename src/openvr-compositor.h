/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_COMPOSITOR_H_
#define GXR_COMPOSITOR_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>

#include <vulkan/vulkan.h>
#include <gulkan.h>

G_BEGIN_DECLS

typedef struct {
  graphene_matrix_t transformation;
  gboolean is_valid;
} GxrPose;

bool
openvr_compositor_get_instance_extensions (GSList **out_list);

bool
openvr_compositor_get_device_extensions (VkPhysicalDevice  physical_device,
                                         GSList          **out_list);

bool
openvr_compositor_gulkan_client_init (GulkanClient *client);

GulkanClient*
openvr_compositor_gulkan_client_new (void);

bool
openvr_compositor_submit (GulkanClient         *client,
                          uint32_t              width,
                          uint32_t              height,
                          VkFormat              format,
                          VkSampleCountFlagBits sample_count,
                          VkImage               left,
                          VkImage               right);

void
openvr_compositor_wait_get_poses (GxrPose *poses, uint32_t count);

G_END_DECLS

#endif /* GXR_COMPOSITOR_H_ */
