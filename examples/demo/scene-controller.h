/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef SCENE_CONTROLLER_H_
#define SCENE_CONTROLLER_H_

#include <glib-object.h>

#include <gxr.h>

#include "scene-pointer-tip.h"
#include "scene-pointer.h"
#include <graphene.h>

#define SCENE_TYPE_CONTROLLER scene_controller_get_type()
G_DECLARE_FINAL_TYPE (SceneController, scene_controller, SCENE, CONTROLLER, GObject)

/**
 * GxrTransformLock:
 * @GXR_TRANSFORM_LOCK_NONE: The grab action does not currently have a transformation it is locked to.
 * @GXR_TRANSFORM_LOCK_PUSH_PULL: Only push pull transformation can be performed.
 * @GXR_TRANSFORM_LOCK_SCALE: Only a scale transformation can be performed.
 *
 * The type of transformation the grab action is currently locked to.
 * This will be detected at the begginging of a grab transformation
 * and reset after the transformation is done.
 *
 **/
typedef enum {
  GXR_TRANSFORM_LOCK_NONE,
  GXR_TRANSFORM_LOCK_PUSH_PULL,
  GXR_TRANSFORM_LOCK_SCALE
} GxrTransformLock;

typedef struct {
  gpointer          hovered_object;
  float             distance;
  graphene_point_t  intersection_2d;
} GxrHoverState;

typedef struct {
  gpointer              grabbed_object;
  graphene_quaternion_t object_rotation;
  graphene_quaternion_t inverse_controller_rotation;
  graphene_point_t      grab_offset;
  GxrTransformLock      transform_lock;
} GxrGrabState;

SceneController *scene_controller_new (GxrController *controller,
                                       GxrContext *context);

ScenePointer *
scene_controller_get_pointer (SceneController *self);

ScenePointerTip *
scene_controller_get_pointer_tip (SceneController *self);

void
scene_controller_set_pointer (SceneController *self, ScenePointer *pointer);

void
scene_controller_set_pointer_tip (SceneController *self, ScenePointerTip *tip);

GxrHoverState *
scene_controller_get_hover_state (SceneController *self);

GxrGrabState *
scene_controller_get_grab_state (SceneController *self);

void
scene_controller_reset_grab_state (SceneController *self);

void
scene_controller_reset_hover_state (SceneController *self);

void
scene_controller_hide_pointer (SceneController *self);

void
scene_controller_show_pointer (SceneController *self);

gboolean
scene_controller_is_pointer_visible (SceneController *self);

void
scene_controller_update_hovered_object (SceneController *self,
                                      gpointer last_object,
                                      gpointer object,
                                      graphene_matrix_t *object_pose,
                                      graphene_point3d_t *intersection_point,
                                      graphene_point_t *intersection_2d,
                                      float intersection_distance);

void
scene_controller_drag_start (SceneController *self,
                           gpointer grabbed_object,
                           graphene_matrix_t *object_pose);

gboolean
scene_controller_get_drag_pose (SceneController     *self,
                              graphene_matrix_t *drag_pose);

void
scene_controller_set_user_data (SceneController *self, gpointer data);

gpointer
scene_controller_get_user_data (SceneController *self);

GxrController *
scene_controller_get_controller (SceneController *self);

#endif /* SCENE_CONTROLLER_H_ */
