/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENVR_ACTION_H_
#define GXR_OPENVR_ACTION_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <graphene.h>

#include "openvr-action-set.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_ACTION openvr_action_get_type()
G_DECLARE_FINAL_TYPE (OpenVRAction, openvr_action, OPENVR, ACTION, GObject)

gboolean
openvr_action_load_manifest (char *path);

OpenVRAction *openvr_action_new (void);

OpenVRAction *
openvr_action_new_from_url (OpenVRActionSet *action_set,
                            char *url);

OpenVRAction *
openvr_action_new_from_type_url (OpenVRActionSet *action_set,
                                 GxrActionType type, char *url);

gboolean
openvr_action_poll (OpenVRAction *self);

gboolean
openvr_action_poll_digital (OpenVRAction *self);

gboolean
openvr_action_poll_analog (OpenVRAction *self);

gboolean
openvr_action_poll_pose_secs_from_now (OpenVRAction *self,
                                       float         secs);

gboolean
openvr_action_trigger_haptic (OpenVRAction *self,
                              float start_seconds_from_now,
                              float duration_seconds,
                              float frequency,
                              float amplitude,
                              guint64 controller_handle);

GxrActionType
openvr_action_get_action_type (OpenVRAction *self);

void
openvr_action_update_input_handles (OpenVRAction *self);

G_END_DECLS

#endif /* GXR_OPENVR_ACTION_H_ */
