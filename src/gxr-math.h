/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_MATH_H_
#define GXR_MATH_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include "openvr-wrapper.h"
#include <graphene.h>
#include <glib.h>

void
gxr_math_print_matrix34 (HmdMatrix34_t mat);

void
gxr_math_graphene_to_matrix34 (graphene_matrix_t *mat, HmdMatrix34_t *mat34);

gboolean
gxr_math_pose_to_matrix (TrackedDevicePose_t *pose,
                         graphene_matrix_t   *transform);

void
gxr_math_matrix34_to_graphene (HmdMatrix34_t *mat34, graphene_matrix_t *mat);

void
gxr_math_matrix44_to_graphene (HmdMatrix44_t *mat44, graphene_matrix_t *mat);


gboolean
gxr_math_direction_from_matrix_vec3 (graphene_matrix_t *matrix,
                                     graphene_vec3_t   *start,
                                     graphene_vec3_t   *direction);

gboolean
gxr_math_direction_from_matrix (graphene_matrix_t *matrix,
                                graphene_vec3_t   *direction);

void
gxr_math_matrix_get_translation (graphene_matrix_t *matrix,
                                 graphene_vec3_t   *vec);


#endif /* GXR_MATH_H_ */
