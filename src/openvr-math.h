/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_MATH_H_
#define OPENVR_GLIB_MATH_H_

#include <openvr_capi.h>
#include <graphene.h>
#include <glib.h>

void
openvr_math_print_matrix34 (HmdMatrix34_t mat);

void
openvr_math_graphene_to_matrix34 (graphene_matrix_t *mat, HmdMatrix34_t *mat34);

gboolean
openvr_math_direction_from_matrix_vec3 (graphene_matrix_t *matrix,
                                        graphene_vec3_t   *start,
                                        graphene_vec3_t   *direction);

gboolean
openvr_math_direction_from_matrix (graphene_matrix_t *matrix,
                                   graphene_vec3_t   *direction);

gboolean
openvr_math_pose_to_matrix (TrackedDevicePose_t *pose,
                            graphene_matrix_t   *transform);

GString *
openvr_math_vec3_to_string (graphene_vec3_t *vec);

void
openvr_math_print_point3d (graphene_point3d_t *point);

void
openvr_math_matrix_get_translation (graphene_matrix_t *matrix,
                                    graphene_vec3_t   *vec);

void
openvr_math_matrix_set_translation (graphene_matrix_t *matrix,
                                    graphene_point3d_t *point);

void
openvr_math_matrix34_to_graphene (HmdMatrix34_t *mat34, graphene_matrix_t *mat);

void
openvr_math_matrix_interpolate (graphene_matrix_t *from,
                                graphene_matrix_t *to,
                                float interpolation,
                                graphene_matrix_t *result);

bool
openvr_math_matrix_equals (graphene_matrix_t *a,
                           graphene_matrix_t *b);

#endif /* OPENVR_GLIB_MATH_H_ */
