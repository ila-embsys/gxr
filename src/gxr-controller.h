/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_CONTROLLER_H_
#define GXR_CONTROLLER_H_

#include <glib-object.h>

#include "gxr-device.h"

#include <graphene.h>

G_BEGIN_DECLS

#define GXR_TYPE_CONTROLLER gxr_controller_get_type ()
G_DECLARE_FINAL_TYPE (GxrController, gxr_controller, GXR, CONTROLLER, GxrDevice)

/**
 * GxrDigitalEvent:
 * @active: Whether or not this action is currently available to be bound in the
 *active action set.
 * @state: Pressed or released.
 * @changed: Whether the state has changed since last event.
 * @controller: The controller identifier.
 * @time: The time of the event.
 *
 * Digital event.
 **/
typedef struct
{
  gboolean       active;
  gboolean       state;
  gboolean       changed;
  GxrController *controller;
  gfloat         time;
} GxrDigitalEvent;

/**
 * GxrAnalogEvent:
 * @active: Whether or not this action is currently available to be bound in the
 *active action set.
 * @state: A #graphene_vec3_t analog state.
 * @delta: State delta since last event.
 * @controller: The controller identifier.
 * @time: The time of the event.
 *
 * Analog event.
 **/
typedef struct
{
  gboolean        active;
  graphene_vec3_t state;
  graphene_vec3_t delta;
  GxrController  *controller;
  gfloat          time;
} GxrAnalogEvent;

/**
 * GxrPoseEvent:
 * @active: Whether or not this action is currently available to be bound in the
 *active action set.
 * @pose: The #graphene_matrix_t pose.
 * @velocity: Velocity
 * @angular_velocity: Angular velocity.
 * @valid: Whether the pose is valid.
 * @device_connected: Whether the device is currently connected.
 * @controller: The controller identifier.
 *
 * Pose event.
 **/
typedef struct
{
  gboolean          active;
  graphene_matrix_t pose;
  graphene_vec3_t   velocity;
  graphene_vec3_t   angular_velocity;
  gboolean          valid;
  gboolean          device_connected;
  GxrController    *controller;
} GxrPoseEvent;

GxrController *
gxr_controller_new (guint64 controller_handle);

void
gxr_controller_get_hand_grip_pose (GxrController     *self,
                                   graphene_matrix_t *pose);

void
gxr_controller_update_pointer_pose (GxrController *self, GxrPoseEvent *event);

void
gxr_controller_update_hand_grip_pose (GxrController *self, GxrPoseEvent *event);

gboolean
gxr_controller_is_pointer_pose_valid (GxrController *self);

gboolean
gxr_controller_get_pointer_pose (GxrController *self, graphene_matrix_t *pose);

G_END_DECLS

#endif /* GXR_CONTROLLER_H_ */
