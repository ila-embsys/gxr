/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_TIME_H_
#define OPENVR_GLIB_TIME_H_

#include <time.h>
#include <glib.h>

#define SEC_IN_MSEC_D 1000.0
#define SEC_IN_NSEC_L 1000000000L
#define SEC_IN_NSEC_D 1000000000.0

void
openvr_time_substract (struct timespec* a,
                       struct timespec* b,
                       struct timespec* out);

double
openvr_time_to_double_secs (struct timespec* time);

void
openvr_time_float_secs_to_timespec (float in, struct timespec* out);

guint32
openvr_time_age_secs_to_monotonic_msecs (float age);

#endif /* OPENVR_GLIB_TIME_H_ */
