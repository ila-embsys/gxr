/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_ACTION_H_
#define OPENVR_GLIB_ACTION_H_

#if !defined (OPENVR_GLIB_INSIDE) && !defined (OPENVR_GLIB_COMPILATION)
#error "Only <openvr-glib.h> can be included directly."
#endif

#include <glib-object.h>
#include <graphene.h>

#include "openvr-wrapper.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_ACTION openvr_action_get_type()
G_DECLARE_FINAL_TYPE (OpenVRAction, openvr_action, OPENVR, ACTION, GObject)

// circular include when including openvr-action-set.h
struct _OpenVRActionSet;

typedef enum OpenVRActionType {
  OPENVR_ACTION_DIGITAL,
  OPENVR_ACTION_ANALOG,
  OPENVR_ACTION_POSE
} OpenVRActionType;

typedef struct OpenVRDigitalEvent {
  gboolean active;
  gboolean state;
  gboolean changed;
  guint64 controller_handle;
  gfloat time;
} OpenVRDigitalEvent;

typedef struct OpenVRAnalogEvent {
  gboolean active;
  graphene_vec3_t state;
  graphene_vec3_t delta;
  guint64 controller_handle;
  gfloat time;
} OpenVRAnalogEvent;

typedef struct OpenVRPoseEvent {
  gboolean active;
  graphene_matrix_t pose;
  graphene_vec3_t velocity;
  graphene_vec3_t angular_velocity;
  gboolean valid;
  gboolean device_connected;
  guint64 controller_handle;
} OpenVRPoseEvent;

gboolean
openvr_action_load_manifest (char *path);

OpenVRAction *openvr_action_new (void);

OpenVRAction *
openvr_action_new_from_url (struct _OpenVRActionSet *action_set,
                            char *url);

OpenVRAction *
openvr_action_new_from_type_url (struct _OpenVRActionSet *action_set,
                                 OpenVRActionType type, char *url);

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
openvr_action_poll_pose_next_frame (OpenVRAction *self);

gboolean
openvr_action_trigger_haptic (OpenVRAction *self,
                              float start_seconds_from_now,
                              float duration_seconds,
                              float frequency,
                              float amplitude);

/* name openvr_action_get_type is already taken by gobject function */
OpenVRActionType
openvr_action_get_action_type (OpenVRAction *self);

void
openvr_action_update_input_handles (OpenVRAction *self);

G_END_DECLS

#endif /* OPENVR_GLIB_ACTION_H_ */
