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

#endif /* GXR_SYSTEM_H_ */
