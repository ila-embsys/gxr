/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_SYSTEM_H_
#define OPENVR_GLIB_SYSTEM_H_

#if !defined (OPENVR_GLIB_INSIDE) && !defined (OPENVR_GLIB_COMPILATION)
#error "Only <openvr-glib.h> can be included directly."
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

#endif /* OPENVR_GLIB_SYSTEM_H_ */
