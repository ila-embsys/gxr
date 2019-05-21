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

#include <openvr_capi.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_ACTION openvr_action_get_type()
G_DECLARE_FINAL_TYPE (OpenVRAction, openvr_action, OPENVR, ACTION, GObject)

/* TODO: figure out if and how more than 2 controllers should be supported */
#define OPENVR_CONTROLLER_COUNT 2

typedef enum OpenVRActionType {
  OPENVR_ACTION_DIGITAL,
  OPENVR_ACTION_ANALOG,
  OPENVR_ACTION_POSE
} OpenVRActionType;

typedef struct OpenVRDigitalEvent {
  gboolean active;
  gboolean state;
  gboolean changed;
  gfloat time;
} OpenVRDigitalEvent;

typedef struct OpenVRAnalogEvent {
  gboolean active;
  graphene_vec3_t state;
  graphene_vec3_t delta;
  gfloat time;
} OpenVRAnalogEvent;

typedef struct OpenVRPoseEvent {
  gboolean active;
  graphene_matrix_t pose;
  graphene_vec3_t velocity;
  graphene_vec3_t angular_velocity;
  gboolean valid;
  gboolean device_connected;
} OpenVRPoseEvent;

gboolean
openvr_action_load_manifest (char *path);

OpenVRAction *openvr_action_new (void);

OpenVRAction *
openvr_action_new_from_url (char *url);

OpenVRAction *
openvr_action_new_from_type_url (OpenVRActionType type, char *url);

gboolean
openvr_action_poll (OpenVRAction *self);

gboolean
openvr_action_poll_digital (OpenVRAction *self);

gboolean
openvr_action_poll_analog (OpenVRAction *self);

gboolean
openvr_action_poll_pose (OpenVRAction *self);

gboolean
openvr_action_trigger_haptic (OpenVRAction *self,
                              float start_seconds_from_now,
                              float duration_seconds,
                              float frequency,
                              float amplitude);

/* name openvr_action_get_type is already taken by gobject function */
OpenVRActionType
openvr_action_get_action_type (OpenVRAction *self);

G_END_DECLS

#endif /* OPENVR_GLIB_ACTION_H_ */
