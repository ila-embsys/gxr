/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_SYSTEM_PRIVATE_H_
#define GXR_SYSTEM_PRIVATE_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib.h>
#include <graphene.h>

#include "openvr-wrapper.h"
#include "gxr-enums.h"

gchar*
openvr_system_get_device_string (TrackedDeviceIndex_t device_index,
                                 ETrackedDeviceProperty property);

GxrEye
openvr_system_eye_to_gxr (EVREye eye);

EVREye
openvr_system_eye_to_openvr (GxrEye eye);

void
openvr_system_get_render_target_size (uint32_t *w, uint32_t *h);

void
openvr_system_get_frustum_angles (GxrEye eye,
                                  float *left, float *right,
                                  float *top, float *bottom);

gboolean
openvr_system_get_hmd_pose (graphene_matrix_t *pose);

gboolean
openvr_system_is_input_available (void);

gboolean
openvr_system_is_tracked_device_connected (uint32_t i);

gboolean
openvr_system_device_is_controller (uint32_t i);

gchar*
openvr_system_get_device_model_name (uint32_t i);

#endif /* GXR_SYSTEM_PRIVATE_H_ */
