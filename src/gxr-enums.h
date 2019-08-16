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

G_BEGIN_DECLS

/**
 * OpenVRAppType:
 * @OPENVR_APP_SCENE: Scene application. Renders stereo viewports for the whole scene.
 * @OPENVR_APP_OVERLAY: Overlay application. Renders mono buffers to overlays.
 * @OPENVR_APP_BACKGROUND: Background application. Does not render anything.
 *
 * Type of OpenVR application.
 *
 **/
typedef enum {
  OPENVR_APP_SCENE = 0,
  OPENVR_APP_OVERLAY,
  OPENVR_APP_BACKGROUND
} OpenVRAppType;

/**
 * OpenVRQuitReason:
 * @VR_QUIT_SHUTDOWN: Runtime is shutting down.
 * @VR_QUIT_APPLICATION_TRANSITION: A new scene application was opened.
 * @VR_QUIT_PROCESS_QUIT: .VR_QUIT_PROCESS_QUIT
 *
 * Reason why an quit signal was received
 *
 **/
typedef enum {
  VR_QUIT_SHUTDOWN,
  VR_QUIT_APPLICATION_TRANSITION,
  VR_QUIT_PROCESS_QUIT
} OpenVRQuitReason;

typedef enum {
  GXR_EYE_LEFT,
  GXR_EYE_RIGHT
} GxrEye;

G_END_DECLS

#endif /* GXR_ENUMS_H_ */
