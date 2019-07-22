/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_ACTION_H_
#define GXR_ACTION_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <graphene.h>

#include "openvr-wrapper.h"
#include "openvr-action-set.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_ACTION openvr_action_get_type()
G_DECLARE_FINAL_TYPE (OpenVRAction, openvr_action, OPENVR, ACTION, GObject)

/**
 * OpenVRDigitalEvent:
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
} OpenVRDigitalEvent;

/**
 * OpenVRAnalogEvent:
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
} OpenVRAnalogEvent;

/**
 * OpenVRPoseEvent:
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
} OpenVRPoseEvent;

gboolean
openvr_action_load_manifest (char *path);

OpenVRAction *openvr_action_new (void);

OpenVRAction *
openvr_action_new_from_url (OpenVRActionSet *action_set,
                            char *url);

OpenVRAction *
openvr_action_new_from_type_url (OpenVRActionSet *action_set,
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

#endif /* GXR_ACTION_H_ */
