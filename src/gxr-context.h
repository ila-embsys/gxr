/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_CONTEXT_H_
#define GXR_CONTEXT_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <graphene.h>
#include <stdint.h>
#include <gulkan.h>

#include "gxr-enums.h"

G_BEGIN_DECLS

#define GXR_TYPE_CONTEXT gxr_context_get_type()
G_DECLARE_DERIVABLE_TYPE (GxrContext, gxr_context, GXR, CONTEXT, GObject)

struct _GxrContextClass
{
  GObjectClass parent;

  gboolean
  (*get_head_pose) (graphene_matrix_t *pose);

  void
  (*get_frustum_angles) (GxrEye eye,
                         float *left, float *right,
                         float *top, float *bottom);

  gboolean
  (*is_input_available) (void);

  void
  (*get_render_dimensions) (GxrContext *context,
                            uint32_t   *width,
                            uint32_t   *height);

  gboolean
  (*is_valid) (GxrContext *self);

  void
  (*poll_event) (GxrContext *self);

  void
  (*show_keyboard) (GxrContext *self);

  gboolean
  (*init_runtime) (GxrContext *self, GxrAppType type);

  gboolean
  (*init_gulkan) (GxrContext *self, GulkanClient *gc);

  gboolean
  (*init_session) (GxrContext *self, GulkanClient *gc);

  gboolean
  (*init_framebuffers) (GxrContext           *self,
                        GulkanFrameBuffer    *framebuffers[2],
                        GulkanClient         *gc,
                        uint32_t              width,
                        uint32_t              height,
                        VkSampleCountFlagBits msaa_sample_count);

  gboolean
  (*submit_framebuffers) (GxrContext           *self,
                          GulkanFrameBuffer    *framebuffers[2],
                          GulkanClient         *gc,
                          uint32_t              width,
                          uint32_t              height,
                          VkSampleCountFlagBits msaa_sample_count);

  uint32_t
  (*get_model_vertex_stride) (GxrContext *self);

  uint32_t
  (*get_model_normal_offset) (GxrContext *self);

  uint32_t
  (*get_model_uv_offset) (GxrContext *self);
};

GxrContext *gxr_context_get_instance (void);

GxrApi
gxr_context_get_api (GxrContext *self);

gboolean
gxr_context_get_head_pose (graphene_matrix_t *pose);

void
gxr_context_get_frustum_angles (GxrEye eye,
                                float *left, float *right,
                                float *top, float *bottom);

gboolean
gxr_context_is_input_available (void);

void
gxr_context_get_render_dimensions (uint32_t *width,
                                   uint32_t *height);

gboolean
gxr_context_is_valid (GxrContext *self);

gboolean
gxr_context_init_runtime (GxrContext *self, GxrAppType type);

gboolean
gxr_context_init_gulkan (GxrContext *self, GulkanClient *gc);

gboolean
gxr_context_init_session (GxrContext *self, GulkanClient *gc);

gboolean
gxr_context_init_framebuffers (GxrContext           *self,
                               GulkanFrameBuffer    *framebuffers[2],
                               GulkanClient         *gc,
                               uint32_t              width,
                               uint32_t              height,
                               VkSampleCountFlagBits msaa_sample_count);

gboolean
gxr_context_submit_framebuffers (GxrContext           *self,
                                 GulkanFrameBuffer    *framebuffers[2],
                                 GulkanClient         *gc,
                                 uint32_t              width,
                                 uint32_t              height,
                                 VkSampleCountFlagBits msaa_sample_count);

void
gxr_context_poll_event (GxrContext *self);

void
gxr_context_show_keyboard (GxrContext *self);

void
gxr_context_emit_keyboard_press (GxrContext *self, gpointer event);

void
gxr_context_emit_keyboard_close (GxrContext *self);

void
gxr_context_emit_quit (GxrContext *self, gpointer event);

void
gxr_context_emit_device_activate (GxrContext *self, gpointer event);

void
gxr_context_emit_device_deactivate (GxrContext *self, gpointer event);

void
gxr_context_emit_device_update (GxrContext *self, gpointer event);

void
gxr_context_emit_bindings_update (GxrContext *self);

void
gxr_context_emit_binding_loaded (GxrContext *self);

void
gxr_context_emit_actionset_update (GxrContext *self);

uint32_t
gxr_context_get_model_vertex_stride (GxrContext *self);

uint32_t
gxr_context_get_model_normal_offset (GxrContext *self);

uint32_t
gxr_context_get_model_uv_offset (GxrContext *self);

G_END_DECLS

#endif /* GXR_CONTEXT_H_ */
