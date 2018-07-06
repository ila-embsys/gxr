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

void
openvr_math_print_matrix34 (HmdMatrix34_t mat);

void
openvr_math_graphene_to_matrix34 (graphene_matrix_t *mat, HmdMatrix34_t *mat34);

#endif /* OPENVR_GLIB_MATH_H_ */
