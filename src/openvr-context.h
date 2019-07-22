/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_CONTEXT_H_
#define GXR_CONTEXT_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>

#include "openvr-wrapper.h"

#include <graphene.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_CONTEXT openvr_context_get_type()
G_DECLARE_FINAL_TYPE (OpenVRContext, openvr_context, OPENVR, CONTEXT, GObject)

struct _OpenVRContext
{
  GObject parent;

  struct VR_IVRSystem_FnTable *system;
  struct VR_IVROverlay_FnTable *overlay;
  struct VR_IVRCompositor_FnTable *compositor;
  struct VR_IVRInput_FnTable *input;
  struct VR_IVRRenderModels_FnTable *model;
  struct VR_IVRApplications_FnTable *applications;
  enum ETrackingUniverseOrigin origin;
};

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

/**
 * OpenVRQuitEvent:
 * @reason: The #OpenVRQuitReason.
 *
 * Event that is emitted when the application needs to quit.
 **/
typedef struct {
  OpenVRQuitReason reason;
} OpenVRQuitEvent;

/**
 * OpenVRDeviceIndexEvent:
 * @controller_handle: A #guint64 controller handle.
 *
 * Event that is emitted when a controller is activated or deaktivated.
 * It carries the handle of a controller.
 **/
typedef struct {
  guint64 controller_handle;
} OpenVRDeviceIndexEvent;

OpenVRContext *openvr_context_get_instance (void);

gboolean
openvr_context_is_valid (OpenVRContext *self);

gboolean
openvr_context_is_installed (void);

gboolean
openvr_context_is_hmd_present (void);

void
openvr_context_print_model_list (OpenVRContext *self);

GSList *
openvr_context_get_model_list (OpenVRContext *self);

const gchar*
openvr_input_error_string (EVRInputError err);

void
openvr_context_poll_event (OpenVRContext *self);

void
openvr_context_show_system_keyboard (OpenVRContext *self);

void
openvr_context_set_system_keyboard_transform (OpenVRContext *self,
                                              graphene_matrix_t *transform);

void
openvr_context_acknowledge_quit (OpenVRContext *self);

gboolean
openvr_context_initialize (OpenVRContext *self, OpenVRAppType type);

gboolean
openvr_context_is_another_scene_running (void);

G_END_DECLS

#endif /* GXR_CONTEXT_H_ */
