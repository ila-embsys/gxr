/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENXR_CONTEXT_H_
#define GXR_OPENXR_CONTEXT_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

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

#include "openxr-interface.h"

G_BEGIN_DECLS

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
                                         uint32_t *width,
                                         uint32_t *height);

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

G_END_DECLS

#endif /* GXR_OPENXR_CONTEXT_H_ */
