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

graphene_matrix_t
openvr_system_get_projection_matrix (GxrEye eye, float near, float far);

graphene_matrix_t
openvr_system_get_eye_to_head_transform (GxrEye eye);

#endif /* GXR_SYSTEM_H_ */
