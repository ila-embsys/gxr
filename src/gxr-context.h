/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_CONTEXT_H_
#define GXR_CONTEXT_H_

#if !defined(GXR_INSIDE) && !defined(GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <graphene.h>
#include <gulkan.h>
#include <stdint.h>

#include <vulkan/vulkan.h>

#define XR_USE_PLATFORM_XLIB 1
#define XR_USE_GRAPHICS_API_VULKAN 1
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "gxr-action-set.h"
#include "gxr-device-manager.h"
#include "gxr-enums.h"
#include "gxr-manifest.h"
#include "gxr-types.h"

G_BEGIN_DECLS

#define GXR_TYPE_CONTEXT gxr_context_get_type ()
G_DECLARE_FINAL_TYPE (GxrContext, gxr_context, GXR, CONTEXT, GObject)

/**
 * GxrContextClass:
 * @parent: The parent class
 */
struct _GxrContextClass
{
  GObjectClass parent;
};

GxrContext *
gxr_context_new (char *app_name, uint32_t app_version);

GxrContext *
gxr_context_new_from_vulkan_extensions (GSList  *instance_ext_list,
                                        GSList  *device_ext_list,
                                        char    *app_name,
                                        uint32_t app_version);

gboolean
gxr_context_get_head_pose (GxrContext *self, graphene_matrix_t *pose);

void
gxr_context_get_frustum_angles (GxrContext *self,
                                GxrEye      eye,
                                float      *left,
                                float      *right,
                                float      *top,
                                float      *bottom);

gboolean
gxr_context_init_framebuffers (GxrContext           *self,
                               VkExtent2D            extent,
                               VkSampleCountFlagBits sample_count,
                               GulkanRenderPass    **render_pass);

void
gxr_context_poll_events (GxrContext *self);

void
gxr_context_get_projection (GxrContext        *self,
                            GxrEye             eye,
                            float              near,
                            float              far,
                            graphene_matrix_t *mat);

void
gxr_context_get_view (GxrContext *self, GxrEye eye, graphene_matrix_t *mat);

gboolean
gxr_context_begin_frame (GxrContext *self);

gboolean
gxr_context_end_frame (GxrContext *self);

gboolean
gxr_context_load_action_manifest (GxrContext *self,
                                  const char *cache_name,
                                  const char *resource_path,
                                  const char *manifest_name);

void
gxr_context_request_quit (GxrContext *self);

GulkanContext *
gxr_context_get_gulkan (GxrContext *self);

gboolean
gxr_context_get_runtime_instance_extensions (GxrContext *self,
                                             GSList    **out_list);

gboolean
gxr_context_get_runtime_device_extensions (GxrContext *self, GSList **out_list);

GxrDeviceManager *
gxr_context_get_device_manager (GxrContext *self);

uint32_t
gxr_context_get_swapchain_length (GxrContext *self);

GulkanFrameBuffer *
gxr_context_get_acquired_framebuffer (GxrContext *self);

GulkanFrameBuffer *
gxr_context_get_framebuffer_at (GxrContext *self, uint32_t i);

VkExtent2D
gxr_context_get_swapchain_extent (GxrContext *self, uint32_t view_index);

uint32_t
gxr_context_get_buffer_index (GxrContext *self);

G_END_DECLS

#endif /* GXR_CONTEXT_H_ */
