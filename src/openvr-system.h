/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_SYSTEM_H_
#define GXR_SYSTEM_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include "openvr-context.h"

#include "openvr-wrapper.h"

void
openvr_system_print_device_info (void);


gchar*
openvr_system_get_device_string (TrackedDeviceIndex_t device_index,
                                 ETrackedDeviceProperty property);

graphene_matrix_t
openvr_system_get_projection_matrix (EVREye eye, float near, float far);

graphene_matrix_t
openvr_system_get_eye_to_head_transform (EVREye eye);

gboolean
openvr_system_get_hmd_pose (graphene_matrix_t *pose);

gboolean
openvr_system_is_input_available (void);

gboolean
openvr_system_is_tracked_device_connected (uint32_t i);

gboolean
openvr_system_device_is_controller (uint32_t i);

void
openvr_system_get_render_target_size (uint32_t *w, uint32_t *h);

void
openvr_system_get_frustum_angles (float *left, float *right,
                                  float *top, float *bottom);

#endif /* GXR_SYSTEM_H_ */
