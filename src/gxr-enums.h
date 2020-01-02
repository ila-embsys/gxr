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

/**
 * GxrAppType:
 * @OPENVR_APP_SCENE: Scene application. Renders stereo viewports for the whole scene.
 * @GXR_APP_OVERLAY: Overlay application. Renders mono buffers to overlays.
 * @OPENVR_APP_BACKGROUND: Background application. Does not render anything.
 *
 * Type of OpenVR application.
 *
 **/
typedef enum {
  GXR_APP_SCENE = 0,
  GXR_APP_OVERLAY,
  GXR_APP_BACKGROUND
} GxrAppType;

/**
 * GxrQuitReason:
 * @VR_QUIT_SHUTDOWN: Runtime is shutting down.
 * @VR_QUIT_APPLICATION_TRANSITION: A new scene application was opened.
 * @VR_QUIT_PROCESS_QUIT: .VR_QUIT_PROCESS_QUIT
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
 * Type of OpenVR viewport.
 *
 **/
typedef enum {
  GXR_EYE_LEFT,
  GXR_EYE_RIGHT
} GxrEye;

/**
 * GxrApi:
 * @GXR_API_OPENVR: Use OpenVR.
 * @GXR_API_OPENXR: Use OpenXR.
 *
 * Type of API backend to use.
 *
 **/
typedef enum {
  GXR_API_OPENVR,
  GXR_API_OPENXR
} GxrApi;

/**
 * GxrActionType:
 * @GXR_ACTION_DIGITAL: A digital action.
 * @GXR_ACTION_ANALOG: An analog action.
 * @GXR_ACTION_POSE: A pose action.
 *
 * The type of the OpenVR action.
 *
 **/
typedef enum {
  GXR_ACTION_DIGITAL,
  GXR_ACTION_ANALOG,
  GXR_ACTION_POSE,
  GXR_ACTION_HAPTIC
} GxrActionType;

#endif /* GXR_ENUMS_H_ */
