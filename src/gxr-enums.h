/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_ENUMS_H_
#define GXR_ENUMS_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#define GXR_DEVICE_INDEX_MAX 64
#define GXR_DEVICE_INDEX_HMD 0
#define GXR_MODEL_NAME_MAX 32768

/**
 * GxrQuitReason:
 * @GXR_QUIT_SHUTDOWN: Runtime is shutting down.
 * @GXR_QUIT_APPLICATION_TRANSITION: A new scene application wants the focus.
 * @GXR_QUIT_PROCESS_QUIT: A running scene application releases the focus.
 *
 * Reason why an quit signal was received
 *
 **/
typedef enum {
  GXR_QUIT_SHUTDOWN,
  GXR_QUIT_APPLICATION_TRANSITION,
  GXR_QUIT_PROCESS_QUIT
} GxrQuitReason;

/**
 * GxrEye:
 * @GXR_EYE_LEFT: Left eye.
 * @GXR_EYE_RIGHT: Right eye.
 *
 * Type of Gxr viewport.
 *
 **/
typedef enum {
  GXR_EYE_LEFT = 0,
  GXR_EYE_RIGHT = 1
} GxrEye;

/**
 * GxrActionType:
 * @GXR_ACTION_DIGITAL: A digital action.
 * @GXR_ACTION_DIGITAL_FROM_FLOAT: A digital action constructed from float thresholds.
 * @GXR_ACTION_VEC2F: An analog action with floats x,y.
 * @GXR_ACTION_FLOAT: An analog action.
 * @GXR_ACTION_POSE: A pose action.
 * @GXR_ACTION_HAPTIC: A haptic action.
 *
 * The type of the GxrAction.
 *
 **/
typedef enum {
  GXR_ACTION_DIGITAL,
  GXR_ACTION_DIGITAL_FROM_FLOAT,
  GXR_ACTION_VEC2F,
  GXR_ACTION_FLOAT,
  GXR_ACTION_POSE,
  GXR_ACTION_HAPTIC
} GxrActionType;

#endif /* GXR_ENUMS_H_ */
