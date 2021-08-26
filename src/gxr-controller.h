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
#include "gxr-context.h"

#include <graphene.h>

G_BEGIN_DECLS

#define GXR_TYPE_CONTROLLER gxr_controller_get_type()
G_DECLARE_FINAL_TYPE (GxrController, gxr_controller, GXR, CONTROLLER, GxrDevice)

GxrController *gxr_controller_new (guint64 controller_handle);

void
gxr_controller_get_hand_grip_pose (GxrController *self,
                                   graphene_matrix_t *pose);

void
gxr_controller_update_pointer_pose (GxrController *self,
                                    GxrPoseEvent  *event);

void
gxr_controller_update_hand_grip_pose (GxrController *self,
                                      GxrPoseEvent  *event);

gboolean
gxr_controller_is_pointer_pose_valid (GxrController *self);

gboolean
gxr_controller_get_pointer_pose (GxrController *self, graphene_matrix_t *pose);

G_END_DECLS

#endif /* GXR_CONTROLLER_H_ */
