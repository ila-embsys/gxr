/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENXR_CONTEXT_H_
#define GXR_OPENXR_CONTEXT_H_

#include <glib.h>
#include <stdbool.h>
#include <glib-object.h>
#include <graphene.h>

#include <vulkan/vulkan.h>

#define XR_USE_PLATFORM_XLIB 1
#define XR_USE_GRAPHICS_API_VULKAN 1
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "gxr-context.h"

G_BEGIN_DECLS

#define OPENXR_TYPE_CONTEXT openxr_context_get_type()
G_DECLARE_FINAL_TYPE (OpenXRContext, openxr_context,
                      OPENXR, CONTEXT, GxrContext)
OpenXRContext *openxr_context_new (void);

void
openxr_context_cleanup(OpenXRContext *self);

bool
openxr_context_begin_frame(OpenXRContext *self);

bool
openxr_context_aquire_swapchain(OpenXRContext *self,
                                uint32_t i,
                                uint32_t *buffer_index);

bool
openxr_context_release_swapchain(OpenXRContext *self,
                                 uint32_t eye);

bool
openxr_context_end_frame(OpenXRContext *self);

XrSwapchainImageVulkanKHR**
openxr_context_get_images(OpenXRContext *self);

void
openxr_context_get_swapchain_dimensions (OpenXRContext *self,
                                         uint32_t i,
                                         VkExtent2D *extent);

void
openxr_context_get_position (OpenXRContext *self,
                             uint32_t i,
                             graphene_vec4_t *v);

VkFormat
openxr_context_get_swapchain_format (OpenXRContext *self);

XrInstance
openxr_context_get_openxr_instance (OpenXRContext *self);

XrSession
openxr_context_get_openxr_session (OpenXRContext *self);

XrSpace
openxr_context_get_tracked_space (OpenXRContext *self);

GSList *
openxr_context_get_manifests (OpenXRContext *self);

G_END_DECLS

#endif /* GXR_OPENXR_CONTEXT_H_ */
