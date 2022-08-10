/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_TYPES_H_
#define GXR_TYPES_H_

#if !defined(GXR_INSIDE) && !defined(GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <stdint.h>

#include <glib.h>
#include <graphene.h>

#include "gxr-enums.h"

#ifndef __GTK_DOC_IGNORE__
struct _GxrController;
typedef struct _GxrController GxrController;
#endif

/**
 * GxrDigitalEvent:
 * @active: Whether or not this action is currently available to be bound in the
 *active action set.
 * @state: Pressed or released.
 * @changed: Whether the state has changed since last event.
 * @controller: The controller identifier.
 * @time: The time of the event.
 *
 * Digital event.
 **/
typedef struct
{
  gboolean       active;
  gboolean       state;
  gboolean       changed;
  GxrController *controller;
  gfloat         time;
} GxrDigitalEvent;

/**
 * GxrAnalogEvent:
 * @active: Whether or not this action is currently available to be bound in the
 *active action set.
 * @state: A #graphene_vec3_t analog state.
 * @delta: State delta since last event.
 * @controller: The controller identifier.
 * @time: The time of the event.
 *
 * Analog event.
 **/
typedef struct
{
  gboolean        active;
  graphene_vec3_t state;
  graphene_vec3_t delta;
  GxrController  *controller;
  gfloat          time;
} GxrAnalogEvent;

/**
 * GxrPoseEvent:
 * @active: Whether or not this action is currently available to be bound in the
 *active action set.
 * @pose: The #graphene_matrix_t pose.
 * @velocity: Velocity
 * @angular_velocity: Angular velocity.
 * @valid: Whether the pose is valid.
 * @device_connected: Whether the device is currently connected.
 * @controller: The controller identifier.
 *
 * Pose event.
 **/
typedef struct
{
  gboolean          active;
  graphene_matrix_t pose;
  graphene_vec3_t   velocity;
  graphene_vec3_t   angular_velocity;
  gboolean          valid;
  gboolean          device_connected;
  GxrController    *controller;
} GxrPoseEvent;

/**
 * GxrStateChangeEvent:
 * @state_change: The #GxrStateChange.
 *
 * Event that is emitted when the application needs to quit.
 **/
typedef struct
{
  GxrStateChange state_change;
} GxrStateChangeEvent;

/**
 * GxrOverlayEvent:
 * @main_session_visible: If a Main session is visible after this event.
 *
 * Event that is emitted when running in OpenXR overlay mode.
 **/
typedef struct
{
  bool main_session_visible;
} GxrOverlayEvent;

/**
 * GxrPose:
 * @transformation: The #graphene_matrix_t.
 * @is_valid: Validity of the pose.
 *
 * A 4x4 matrix pose.
 **/
typedef struct
{
  graphene_matrix_t transformation;
  gboolean          is_valid;
} GxrPose;

#endif /* GXR_TYPES_H_ */
