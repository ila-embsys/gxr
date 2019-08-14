/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-time.h"
#include <glib/gprintf.h>

/* assuming a > b */
void
gxr_time_substract (struct timespec* a,
                    struct timespec* b,
                    struct timespec* out)
{
  out->tv_sec = a->tv_sec - b->tv_sec;

  if (a->tv_nsec < b->tv_nsec)
  {
    out->tv_nsec = a->tv_nsec + SEC_IN_NSEC_L - b->tv_nsec;
    out->tv_sec--;
  }
  else
  {
    out->tv_nsec = a->tv_nsec - b->tv_nsec;
  }
}

double
gxr_time_to_double_secs (struct timespec* time)
{
  return ((double) time->tv_sec + (time->tv_nsec / SEC_IN_NSEC_D));
}

void
gxr_time_float_secs_to_timespec (float in, struct timespec* out)
{
  out->tv_sec = (time_t) in;
  out->tv_nsec = (long) ((in - (float) out->tv_sec) * SEC_IN_NSEC_D);
}

guint32
gxr_time_age_secs_to_monotonic_msecs (float age)
{
  struct timespec mono_time;
  if (clock_gettime (CLOCK_MONOTONIC, &mono_time) != 0)
  {
    g_printerr ("Could not read system clock\n");
    return 0;
  }

  struct timespec event_age;
  gxr_time_float_secs_to_timespec (age, &event_age);

  struct timespec event_age_on_mono;
  gxr_time_substract (&mono_time, &event_age, &event_age_on_mono);

  double time_s = gxr_time_to_double_secs (&event_age_on_mono);

  return (guint32) (time_s * SEC_IN_MSEC_D);
}

