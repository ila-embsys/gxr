/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_TYPES_H_
#define GXR_TYPES_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <stdint.h>

#include <glib.h>
#include <graphene.h>

#include "gxr-enums.h"

/**
 * GxrDigitalEvent:
 * @active: Whether or not this action is currently available to be bound in the active action set.
 * @state: Pressed or released.
 * @changed: Whether the state has changed since last event.
 * @controller_handle: The controller identifier.
 * @time: The time of the event.
 *
 * Digital event.
 **/
typedef struct {
  gboolean active;
  gboolean state;
  gboolean changed;
  guint64 controller_handle;
  gfloat time;
} GxrDigitalEvent;

/**
 * GxrAnalogEvent:
 * @active: Whether or not this action is currently available to be bound in the active action set.
 * @state: A #graphene_vec3_t analog state.
 * @delta: State delta since last event.
 * @controller_handle: The controller identifier.
 * @time: The time of the event.
 *
 * Analog event.
 **/
typedef struct {
  gboolean active;
  graphene_vec3_t state;
  graphene_vec3_t delta;
  guint64 controller_handle;
  gfloat time;
} GxrAnalogEvent;

/**
 * GxrPoseEvent:
 * @active: Whether or not this action is currently available to be bound in the active action set.
 * @pose: The #graphene_matrix_t pose.
 * @velocity: Velocity
 * @angular_velocity: Angular velocity.
 * @valid: Whether the pose is valid.
 * @device_connected: Whether the device is currently connected.
 * @controller_handle: The controller identifier.
 *
 * Pose event.
 **/
typedef struct {
  gboolean active;
  graphene_matrix_t pose;
  graphene_vec3_t velocity;
  graphene_vec3_t angular_velocity;
  gboolean valid;
  gboolean device_connected;
  guint64 controller_handle;
} GxrPoseEvent;

/**
 * GxrQuitEvent:
 * @reason: The #GxrQuitReason.
 *
 * Event that is emitted when the application needs to quit.
 **/
typedef struct {
  GxrQuitReason reason;
} GxrQuitEvent;

/**
 * GxrDeviceIndexEvent:
 * @controller_handle: A #guint64 controller handle.
 *
 * Event that is emitted when a controller is activated or deactivated.
 * It carries the handle of a controller.
 **/
typedef struct {
  guint64 controller_handle;
} GxrDeviceIndexEvent;

/**
 * GxrPose:
 * @transformation: The #graphene_matrix_t.
 * @is_valid: Validity of the pose.
 *
 * A 4x4 matrix pose.
 **/
typedef struct {
  graphene_matrix_t transformation;
  gboolean is_valid;
} GxrPose;

/**
 * GxrPixelSize:
 * @width: Width
 * @height: Height
 *
 * Size in pixels.
 **/
typedef struct {
  uint32_t width;
  uint32_t height;
} GxrPixelSize;

#endif /* GXR_TYPES_H_ */
