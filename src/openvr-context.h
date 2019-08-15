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
#include <graphene.h>

#include "gxr-enums.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_CONTEXT openvr_context_get_type()
G_DECLARE_FINAL_TYPE (OpenVRContext, openvr_context, OPENVR, CONTEXT, GObject)

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
