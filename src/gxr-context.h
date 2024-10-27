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

#include "gxr-device-manager.h"

G_BEGIN_DECLS

#define GXR_DEVICE_INDEX_MAX 64

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

/**
 * GxrEye:
 * @GXR_EYE_LEFT: Left eye.
 * @GXR_EYE_RIGHT: Right eye.
 *
 * Type of Gxr viewport.
 *
 **/
typedef enum
{
  GXR_EYE_LEFT = 0,
  GXR_EYE_RIGHT = 1
} GxrEye;

/**
 * GxrStateChange:
 * @GXR_STATE_FRAMECYCLE_START: Ready to call gxr_context_begin_frame /
 *  gxr_context_end_frame.
 * @GXR_STATE_FRAMECYCLE_STOP: Not ready to call gxr_context_begin_frame /
 *  gxr_context_end_frame.
 * @GXR_STATE_RENDERING_START: The frame content will be shown in XR.
 * @GXR_STATE_RENDERING_STOP: The frame content will not be visible, expensive
 *  rendering work can be skipped, but  gxr_context_begin_frame /
 *   gxr_context_end_frame should be called.
 * @GXR_STATE_SHUTDOWN: XR Runtime is shutting down.
 *
 **/
typedef enum
{
  GXR_STATE_FRAMECYCLE_START,
  GXR_STATE_FRAMECYCLE_STOP,
  GXR_STATE_RENDERING_START,
  GXR_STATE_RENDERING_STOP,
  GXR_STATE_SHUTDOWN,
} GxrStateChange;

/**
 * GxrStateChangeEvent:
 * @state_change: The #GxrStateChange.
 *
 * Event that is emitted when the application needs to quit.
 **/
typedef struct
{
  GxrStateChange state_change;
} GxrStateChangeEvent;

/**
 * GxrOverlayEvent:
 * @main_session_visible: If a Main session is visible after this event.
 *
 * Event that is emitted when running in OpenXR overlay mode.
 **/
typedef struct
{
  bool main_session_visible;
} GxrOverlayEvent;

/**
 * gxr_context_new:
 * @app_name: The name of the application.
 * @app_version: The version of the application.
 * @returns: A newly allocated #GxrContext.
 *
 * Creates a new GXR context.
 */
GxrContext *
gxr_context_new (char *app_name, uint32_t app_version);

/**
 * gxr_context_new_from_vulkan_extensions:
 * @instance_ext_list: (element-type GString): A list of Vulkan instance extensions.
 * @device_ext_list: (element-type GString): A list of Vulkan device extensions.
 * @app_name: The name of the application.
 * @app_version: The version of the application.
 * @returns: A newly allocated #GxrContext.
 *
 * Creates a new GXR context using the specified Vulkan extensions.
 */
GxrContext *
gxr_context_new_from_vulkan_extensions (GSList  *instance_ext_list,
                                        GSList  *device_ext_list,
                                        char    *app_name,
                                        uint32_t app_version);

/**
 * gxr_context_get_head_pose:
 * @self: A #GxrContext.
 * @pose: (out): A pointer to a graphene_matrix_t to store the pose.
 * @returns: `TRUE` if the pose was retrieved successfully, `FALSE` otherwise.
 *
 * Retrieves the current head pose.
 */
gboolean
gxr_context_get_head_pose (GxrContext *self, graphene_matrix_t *pose);

/**
 * gxr_context_get_frustum_angles:
 * @self: A #GxrContext.
 * @eye: The eye for which to get the frustum angles.
 * @left: (out): A pointer to store the left frustum angle.
 * @right: (out): A pointer to store the right frustum angle.
 * @top: (out): A pointer to store the top frustum angle.
 * @bottom: (out): A pointer to store the bottom frustum angle.
 *
 * Retrieves the frustum angles for the specified eye.
 */
void
gxr_context_get_frustum_angles (GxrContext *self,
                                GxrEye      eye,
                                float      *left,
                                float      *right,
                                float      *top,
                                float      *bottom);

/**
 * gxr_context_init_framebuffers:
 * @self: A #GxrContext.
 * @extent: (type VkExtent2D): The extent of the framebuffers.
 * @sample_count: (type VkSampleCountFlagBits): The sample count for the framebuffers.
 * @render_pass: (out): (type GVulkanRenderPass**): A pointer to store the render pass.
 * @returns: `TRUE` if the framebuffers were initialized successfully, `FALSE` otherwise.
 *
 * Initializes the framebuffers for the context.
 */
gboolean
gxr_context_init_framebuffers (GxrContext           *self,
                               VkExtent2D            extent,
                               VkSampleCountFlagBits sample_count,
                               GulkanRenderPass    **render_pass);

/**
 * gxr_context_poll_events:
 * @self: A #GxrContext.
 *
 * Polls for events from the context.
 */
void
gxr_context_poll_events (GxrContext *self);

/**
 * gxr_context_get_projection:
 * @self: A #GxrContext.
 * @eye: The eye for which to get the projection matrix.
 * @near: The near clipping plane distance.
 * @far: The far clipping plane distance.
 * @mat: (out): A pointer to store the projection matrix.
 *
 * Retrieves the projection matrix for the specified eye.
 */
void
gxr_context_get_projection (GxrContext        *self,
                            GxrEye             eye,
                            float              near,
                            float              far,
                            graphene_matrix_t *mat);

/**
 * gxr_context_get_view:
 * @self: A #GxrContext.
 * @eye: The eye for which to get the view matrix.
 * @mat: (out): A pointer to store the view matrix.
 *
 * Retrieves the view matrix for the specified eye.
 */
void
gxr_context_get_view (GxrContext *self, GxrEye eye, graphene_matrix_t *mat);

/**
 * gxr_context_get_eye_position:
 * @self: A #GxrContext.
 * @eye: The eye for which to get the position.
 * @v: (out): A pointer to store the eye position.
 *
 * Retrieves the position of the specified eye.
 */
void
gxr_context_get_eye_position (GxrContext *self, GxrEye eye, graphene_vec3_t *v);

/**
 * gxr_context_wait_frame:
 * @self: A #GxrContext.
 * @returns: `TRUE` if the frame was acquired successfully, `FALSE` otherwise.
 *
 * Waits for a frame to be acquired.
 */
gboolean
gxr_context_wait_frame (GxrContext *self);

/**
 * gxr_context_begin_frame:
 * @self: A #GxrContext.
 * @returns: `TRUE` if the frame was begun successfully, `FALSE` otherwise.
 *
 * Begins a new frame.
 */
gboolean
gxr_context_begin_frame (GxrContext *self);

/**
 * gxr_context_end_frame:
 * @self: A #GxrContext.
 * @returns: `TRUE` if the frame was ended successfully, `FALSE` otherwise.
 *
 * Ends the current frame.
 */
gboolean
gxr_context_end_frame (GxrContext *self);

/**
 * gxr_context_request_quit:
 * @self: A #GxrContext.
 *
 * Requests that the context be quit.
 */
void
gxr_context_request_quit (GxrContext *self);

/**
 * gxr_context_get_gulkan:
 * @self: A #GxrContext.
 * @returns: (type GVulkanContext*): The Vulkan context.
 */
GulkanContext *
gxr_context_get_gulkan (GxrContext *self);

/**
 * gxr_context_get_runtime_instance_extensions:
 * @self: A #GxrContext.
 * @out_list: (element-type GString) (out): A pointer to store the list of instance extensions.
 * @returns: `TRUE` if the instance extensions were retrieved successfully, `FALSE` otherwise.
 *
 * Retrieves the list of instance extensions used by the context.
 */
gboolean
gxr_context_get_runtime_instance_extensions (GxrContext *self,
                                             GSList    **out_list);

/**
 * gxr_context_get_runtime_device_extensions:
 * @self: A #GxrContext.
 * @out_list: (element-type GString) (out): A pointer to store the list of device extensions.
 * @returns: `TRUE` if the device extensions were retrieved successfully, `FALSE` otherwise.
 *
 * Retrieves the list of device extensions used by the context.
 */
gboolean
gxr_context_get_runtime_device_extensions (GxrContext *self, GSList **out_list);

/**
 * gxr_context_get_device_manager:
 * @self: A #GxrContext.
 * @returns: (transfer none): The device manager.
 *
 * Retrieves the device manager associated with the context.
 */
GxrDeviceManager *
gxr_context_get_device_manager (GxrContext *self);

/**
 * gxr_context_get_swapchain_length:
 * @self: A #GxrContext.
 * @returns: The length of the swapchain.
 */
uint32_t
gxr_context_get_swapchain_length (GxrContext *self);

/**
 * gxr_context_get_acquired_framebuffer:
 * @self: A #GxrContext.
 * @returns: (type GVulkanFrameBuffer*): The acquired framebuffer.
 */
GulkanFrameBuffer *
gxr_context_get_acquired_framebuffer (GxrContext *self);

/**
 * gxr_context_get_framebuffer_at:
 * @self: A #GxrContext.
 * @i: The index of the framebuffer.
 * @returns: (type GVulkanFrameBuffer*): The framebuffer at the specified index.
 */
GulkanFrameBuffer *
gxr_context_get_framebuffer_at (GxrContext *self, uint32_t i);

/**
 * gxr_context_get_swapchain_extent:
 * @self: A #GxrContext.
 * @view_index: The index of the view.
 * @returns: (type VkExtent2D): The extent of the swapchain for the specified view.
 */
VkExtent2D
gxr_context_get_swapchain_extent (GxrContext *self, uint32_t view_index);

/**
 * gxr_context_get_buffer_index:
 * @self: A #GxrContext.
 * @returns: The index of the current buffer.
 */
uint32_t
gxr_context_get_buffer_index (GxrContext *self);

/**
 * gxr_context_attach_action_sets:
 * @self: A #GxrContext.
 * @sets: An array of action sets.
 * @count: The number of action sets.
 * @returns: `TRUE` if the action sets were attached successfully, `FALSE` otherwise.
 *
 * Attaches the specified action sets to the context.
 */
gboolean
gxr_context_attach_action_sets (GxrContext    *self,
                                GxrActionSet **sets,
                                uint32_t       count);

G_END_DECLS

#endif /* GXR_CONTEXT_H_ */
