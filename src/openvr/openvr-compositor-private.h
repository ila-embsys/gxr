/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_CONTEXT_COMPOSITOR_PRIVATE_H_
#define GXR_CONTEXT_COMPOSITOR_PRIVATE_H_

#include "openvr-wrapper.h"
#include "openvr-context.h"

G_BEGIN_DECLS

enum ETrackingUniverseOrigin
openvr_compositor_get_tracking_space ();

bool
openvr_compositor_gulkan_client_init (GulkanClient *client);

bool
openvr_compositor_submit (GulkanClient         *client,
                          uint32_t              width,
                          uint32_t              height,
                          VkFormat              format,
                          VkSampleCountFlagBits sample_count,
                          VkImage               left,
                          VkImage               right);

G_END_DECLS

#endif /* GXR_CONTEXT_COMPOSITOR_PRIVATE_H_ */
