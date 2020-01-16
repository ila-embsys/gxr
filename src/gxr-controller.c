/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-controller.h"
#include "gxr-pointer.h"

struct _GxrController
{
  GObject parent;

  gpointer user_pointer;

  guint64 controller_handle;
  GxrPointer *pointer_ray;
  GxrPointerTip *pointer_tip;
  GxrHoverState hover_state;
  GxrGrabState grab_state;

  graphene_matrix_t pose_hand_grip;
  graphene_matrix_t pose_pointer;

  graphene_matrix_t intersection_pose;
  GxrContext *context;
};

G_DEFINE_TYPE (GxrController, gxr_controller, G_TYPE_OBJECT)

static void
gxr_controller_finalize (GObject *gobject);

static void
gxr_controller_class_init (GxrControllerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gxr_controller_finalize;
}

static void
gxr_controller_init (GxrController *self)
{
  self->hover_state.distance = 1.0f;
  self->grab_state.transform_lock = GXR_TRANSFORM_LOCK_NONE;

  self->grab_state.grabbed_object = NULL;
  self->hover_state.hovered_object = NULL;

  graphene_matrix_init_identity (&self->pose_pointer);
  graphene_matrix_init_identity (&self->pose_hand_grip);
  self->pointer_ray = NULL;
  self->pointer_tip = NULL;

  self->context = NULL;
  self->user_pointer = NULL;
}

void
gxr_controller_set_user_pointer (GxrController *self, gpointer ptr)
{
  self->user_pointer = ptr;
}

gpointer
gxr_controller_get_user_pointer (GxrController *self)
{
  return self->user_pointer;
}

GxrController *
gxr_controller_new (guint64 controller_handle,
                    GxrContext *context)
{
  GxrController *controller =
    (GxrController*) g_object_new (GXR_TYPE_CONTROLLER, 0);
  controller->controller_handle = controller_handle;
  controller->context = context;
  return controller;
}

static void
gxr_controller_finalize (GObject *gobject)
{
  GxrController *self = GXR_CONTROLLER (gobject);
  if (self->pointer_ray)
    g_object_unref (self->pointer_ray);
  if (self->pointer_tip)
    g_object_unref (self->pointer_tip);
  (void) self;
}

GxrPointer *
gxr_controller_get_pointer (GxrController *self)
{
  return self->pointer_ray;
}

GxrPointerTip *
gxr_controller_get_pointer_tip (GxrController *self)
{
  return self->pointer_tip;
}

void
gxr_controller_set_pointer (GxrController *self, GxrPointer *pointer)
{
  self->pointer_ray = pointer;
}

void
gxr_controller_set_pointer_tip (GxrController *self, GxrPointerTip *tip)
{
  self->pointer_tip = tip;
}

guint64
gxr_controller_get_handle (GxrController *self)
{
  return self->controller_handle;
}

GxrHoverState *
gxr_controller_get_hover_state (GxrController *self)
{
  return &self->hover_state;
}

GxrGrabState *
gxr_controller_get_grab_state (GxrController *self)
{
  return &self->grab_state;
}

void
gxr_controller_reset_grab_state (GxrController *self)
{
  self->grab_state.grabbed_object = NULL;
  graphene_point3d_init (&self->grab_state.grab_offset, 0, 0, 0);
  graphene_quaternion_init_identity (
    &self->grab_state.inverse_controller_rotation);
  self->grab_state.transform_lock = GXR_TRANSFORM_LOCK_NONE;
}

void
gxr_controller_reset_hover_state (GxrController *self)
{
  self->hover_state.hovered_object = NULL;
  graphene_point_init (&self->hover_state.intersection_2d, 0, 0);
  self->hover_state.distance = 1.0;
}

void
gxr_controller_update_hand_grip_pose (GxrController *self,
                                      graphene_matrix_t *pose)
{
  graphene_matrix_init_from_matrix (&self->pose_hand_grip, pose);
}

void
gxr_controller_get_hand_grip_pose (GxrController *self,
                                   graphene_matrix_t *pose)
{
  graphene_matrix_init_from_matrix (pose, &self->pose_hand_grip);
}

void
gxr_controller_update_pointer_pose (GxrController *self,
                                    graphene_matrix_t *pose)
{
  graphene_matrix_init_from_matrix (&self->pose_pointer, pose);
  gxr_pointer_move (self->pointer_ray, pose);
}

void
gxr_controller_hide_pointer (GxrController *self)
{
  gxr_pointer_tip_hide (self->pointer_tip);
}

void
gxr_controller_show_pointer (GxrController *self)
{
  gxr_pointer_tip_show (self->pointer_tip);
}

gboolean
gxr_controller_is_pointer_visible (GxrController *self)
{
  return gxr_pointer_tip_is_visible (self->pointer_tip);
}
