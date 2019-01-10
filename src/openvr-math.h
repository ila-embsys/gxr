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
openvr_math_matrix44_to_graphene (HmdMatrix44_t *mat44, graphene_matrix_t *mat);

void
openvr_math_matrix_interpolate (graphene_matrix_t *from,
                                graphene_matrix_t *to,
                                float interpolation,
                                graphene_matrix_t *result);

void
openvr_math_worldspace_to_screenspace (graphene_point3d_t *worldspace_point,
                                       graphene_matrix_t  *camera_transform,
                                       graphene_matrix_t  *projection_matrix,
                                       graphene_point3d_t *screenspace_point,
                                       float              *w);

void
openvr_math_screenspace_to_worldspace (graphene_point3d_t *screenspace_point,
                                       graphene_matrix_t  *camera_transform,
                                       graphene_matrix_t  *projection_matrix,
                                       graphene_point3d_t *worldspace_point,
                                       float              *w);

bool
openvr_math_matrix_equals (graphene_matrix_t *a,
                           graphene_matrix_t *b);

float
openvr_math_point_matrix_distance (graphene_point3d_t *intersection_point,
                                   graphene_matrix_t  *pose);

#endif /* OPENVR_GLIB_MATH_H_ */
