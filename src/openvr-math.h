/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_MATH_H_
#define OPENVR_GLIB_MATH_H_

#if !defined (OPENVR_GLIB_INSIDE) && !defined (OPENVR_GLIB_COMPILATION)
#error "Only <openvr-glib.h> can be included directly."
#endif

#include "openvr-wrapper.h"
#include <graphene.h>
#include <glib.h>

void
openvr_math_print_matrix34 (HmdMatrix34_t mat);

void
openvr_math_graphene_to_matrix34 (graphene_matrix_t *mat, HmdMatrix34_t *mat34);

gboolean
openvr_math_pose_to_matrix (TrackedDevicePose_t *pose,
                            graphene_matrix_t   *transform);

void
openvr_math_matrix34_to_graphene (HmdMatrix34_t *mat34, graphene_matrix_t *mat);

void
openvr_math_matrix44_to_graphene (HmdMatrix44_t *mat44, graphene_matrix_t *mat);


gboolean
openvr_math_direction_from_matrix_vec3 (graphene_matrix_t *matrix,
                                        graphene_vec3_t   *start,
                                        graphene_vec3_t   *direction);

gboolean
openvr_math_direction_from_matrix (graphene_matrix_t *matrix,
                                   graphene_vec3_t   *direction);

void
openvr_math_matrix_get_translation (graphene_matrix_t *matrix,
                                    graphene_vec3_t   *vec);


#endif /* OPENVR_GLIB_MATH_H_ */
