/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-controller.h"

#include "graphene-ext.h"
#include "gxr-device.h"

struct _GxrController
{
  GxrDevice parent;

  graphene_matrix_t pointer_pose;
  gboolean pointer_pose_valid;

  graphene_matrix_t hand_grip_pose;
  gboolean hand_grip_pose_valid;
};

G_DEFINE_TYPE (GxrController, gxr_controller, GXR_TYPE_DEVICE)

enum {
  MOVE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
gxr_controller_finalize (GObject *gobject);

static void
gxr_controller_class_init (GxrControllerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gxr_controller_finalize;

  signals[MOVE] = g_signal_new ("move",
                                G_TYPE_FROM_CLASS (klass),
                                G_SIGNAL_RUN_FIRST,
                                0, NULL, NULL, NULL, G_TYPE_NONE,
                                1, G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static void
gxr_controller_init (GxrController *self)
{
  graphene_matrix_init_identity (&self->pointer_pose);
  graphene_matrix_init_identity (&self->hand_grip_pose);

  self->pointer_pose_valid = FALSE;
  self->hand_grip_pose_valid = FALSE;
}

GxrController *
gxr_controller_new (guint64 controller_handle)
{
  GxrController *controller =
    (GxrController*) g_object_new (GXR_TYPE_CONTROLLER, 0);

  GxrDevice *device = GXR_DEVICE (controller);
  gxr_device_set_handle (device, controller_handle);

  return controller;
}

static void
gxr_controller_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (gxr_controller_parent_class)->finalize (gobject);
}

void
gxr_controller_get_hand_grip_pose (GxrController *self,
                                   graphene_matrix_t *pose)
{
  graphene_matrix_init_from_matrix (pose, &self->hand_grip_pose);
}

void
gxr_controller_update_pointer_pose (GxrController     *self,
                                    graphene_matrix_t *pose,
                                    gboolean           valid)
{
  graphene_matrix_init_from_matrix (&self->pointer_pose, pose);
  self->pointer_pose_valid = valid;
  g_signal_emit (self, signals[MOVE], 0);
}

void
gxr_controller_update_hand_grip_pose (GxrController     *self,
                                      graphene_matrix_t *pose,
                                      gboolean           valid)
{
  graphene_matrix_init_from_matrix (&self->hand_grip_pose, pose);
  self->hand_grip_pose_valid = valid;
}

gboolean
gxr_controller_is_pointer_pose_valid (GxrController *self)
{
  return self->pointer_pose_valid;
}

gboolean
gxr_controller_get_pointer_pose (GxrController *self, graphene_matrix_t *pose)
{
  graphene_matrix_init_from_matrix (pose, &self->pointer_pose);
  return self->pointer_pose_valid;
}
