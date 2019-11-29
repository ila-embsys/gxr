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

#include <stdint.h>

#include <glib.h>
#include <graphene.h>

#include "gxr-enums.h"

void
openvr_system_print_device_info (void);

graphene_matrix_t
openvr_system_get_projection_matrix (GxrEye eye, float near, float far);

graphene_matrix_t
openvr_system_get_eye_to_head_transform (GxrEye eye);

gboolean
openvr_system_is_tracked_device_connected (uint32_t i);

gboolean
openvr_system_device_is_controller (uint32_t i);

gchar*
openvr_system_get_device_model_name (uint32_t i);

#endif /* GXR_SYSTEM_H_ */
