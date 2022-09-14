/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "scene-controller.h"

#include "graphene-ext.h"

struct _SceneController
{
  GObject parent;

  ScenePointer *pointer_ray;

  GxrContext    *context;
  GxrController *controller;

  gulong controller_move_signal;
};

G_DEFINE_TYPE (SceneController, scene_controller, G_TYPE_OBJECT)

static void
scene_controller_finalize (GObject *gobject);

static void
scene_controller_class_init (SceneControllerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = scene_controller_finalize;
}

static void
scene_controller_init (SceneController *self)
{
  self->pointer_ray = NULL;
  self->context = NULL;
  self->controller = NULL;
  self->controller_move_signal = 0;
}

static void
_controller_move_cb (GxrController       *controller,
                     GxrStateChangeEvent *event,
                     SceneController     *self);

SceneController *
scene_controller_new (GxrController *controller, GxrContext *context)
{
  SceneController *self = (SceneController *)
    g_object_new (SCENE_TYPE_CONTROLLER, 0);

  self->context = g_object_ref (context);
  self->controller = g_object_ref (controller);

  self->controller_move_signal = g_signal_connect (self->controller, "move",
                                                   (GCallback)
                                                     _controller_move_cb,
                                                   self);

  return self;
}

static void
scene_controller_finalize (GObject *gobject)
{
  SceneController *self = SCENE_CONTROLLER (gobject);

  if (self->pointer_ray)
    g_object_unref (self->pointer_ray);

  g_signal_handler_disconnect (self->controller, self->controller_move_signal);
  g_object_unref (self->controller);
  g_object_unref (self->context);

  g_debug ("Destroyed pointer ray, pointer tip, controller\n");

  G_OBJECT_CLASS (scene_controller_parent_class)->finalize (gobject);
}

ScenePointer *
scene_controller_get_pointer (SceneController *self)
{
  return self->pointer_ray;
}

void
scene_controller_set_pointer (SceneController *self, ScenePointer *pointer)
{
  self->pointer_ray = pointer;
}

static void
_controller_move_cb (GxrController       *controller,
                     GxrStateChangeEvent *event,
                     SceneController     *self)
{
  (void) event;
  (void) self;

  graphene_matrix_t pose;
  gxr_controller_get_pointer_pose (controller, &pose);

  /* The pointer's pose is always set by pose update. When hovering, only the
   * length is set by gxr_controller_update_hovered_object(). */
  scene_object_set_transformation_direct (SCENE_OBJECT (self->pointer_ray),
                                          &pose);
}

GxrController *
scene_controller_get_controller (SceneController *self)
{
  return self->controller;
}
