/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "scene-controller.h"
#include "demo-pointer.h"

#include "graphene-ext.h"

struct _SceneController
{
  GObject parent;

  DemoPointer *pointer_ray;
  DemoPointerTip *pointer_tip;
  GxrHoverState hover_state;
  GxrGrabState grab_state;

  graphene_matrix_t intersection_pose;

  GxrContext *context;
  GxrController *controller;

  gulong controller_move_signal;

  gpointer user_data;
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
  self->hover_state.distance = 1.0f;
  self->grab_state.transform_lock = GXR_TRANSFORM_LOCK_NONE;

  self->grab_state.grabbed_object = NULL;
  self->hover_state.hovered_object = NULL;

  self->pointer_ray = NULL;
  self->pointer_tip = NULL;

  self->user_data = NULL;

  self->context = NULL;
  self->controller = NULL;
  self->controller_move_signal = 0;
}

static void
_controller_move_cb (GxrController *controller,
                     GxrQuitEvent *event,
                     SceneController *self);

SceneController *
scene_controller_new (GxrController *controller,
                      GxrContext *context)
{
  SceneController *self =
    (SceneController*) g_object_new (SCENE_TYPE_CONTROLLER, 0);

  self->context = g_object_ref (context);
  self->controller = g_object_ref (controller);

  self->controller_move_signal =
    g_signal_connect (self->controller, "move",
                      (GCallback) _controller_move_cb, self);

  return self;
}

static void
scene_controller_finalize (GObject *gobject)
{
  SceneController *self = SCENE_CONTROLLER (gobject);

  if (self->pointer_ray)
    g_object_unref (self->pointer_ray);
  if (self->pointer_tip)
    g_object_unref (self->pointer_tip);

  g_signal_handler_disconnect(self->controller, self->controller_move_signal);
  g_object_unref (self->controller);
  g_object_unref (self->context);

  g_debug ("Destroyed pointer ray, pointer tip, controller\n");

  G_OBJECT_CLASS (scene_controller_parent_class)->finalize (gobject);
}

DemoPointer *
scene_controller_get_pointer (SceneController *self)
{
  return self->pointer_ray;
}

DemoPointerTip *
scene_controller_get_pointer_tip (SceneController *self)
{
  return self->pointer_tip;
}

void
scene_controller_set_pointer (SceneController *self, DemoPointer *pointer)
{
  self->pointer_ray = pointer;
}

void
scene_controller_set_pointer_tip (SceneController *self, DemoPointerTip *tip)
{
  self->pointer_tip = tip;
}

GxrHoverState *
scene_controller_get_hover_state (SceneController *self)
{
  return &self->hover_state;
}

GxrGrabState *
scene_controller_get_grab_state (SceneController *self)
{
  return &self->grab_state;
}

void
scene_controller_reset_grab_state (SceneController *self)
{
  self->grab_state.grabbed_object = NULL;
  graphene_point_init (&self->grab_state.grab_offset, 0, 0);
  graphene_quaternion_init_identity (
    &self->grab_state.inverse_controller_rotation);
  self->grab_state.transform_lock = GXR_TRANSFORM_LOCK_NONE;
}

void
scene_controller_reset_hover_state (SceneController *self)
{
  self->hover_state.hovered_object = NULL;
  graphene_point_init (&self->hover_state.intersection_2d, 0, 0);
  self->hover_state.distance = 1.0;
}

static void
_no_hover_transform_tip (SceneController *self,
                         graphene_matrix_t *controller_pose)
{
  graphene_point3d_t distance_translation_point;
  graphene_point3d_init (&distance_translation_point, 0.f, 0.f, -5.0f);

  graphene_matrix_t tip_pose;

  graphene_quaternion_t controller_rotation;
  graphene_quaternion_init_from_matrix (&controller_rotation, controller_pose);

  graphene_point3d_t controller_translation_point;
  graphene_ext_matrix_get_translation_point3d (controller_pose,
                                               &controller_translation_point);

  graphene_matrix_init_identity (&tip_pose);
  graphene_matrix_translate (&tip_pose, &distance_translation_point);
  graphene_matrix_rotate_quaternion (&tip_pose, &controller_rotation);
  graphene_matrix_translate (&tip_pose, &controller_translation_point);

  demo_pointer_tip_set_transformation (self->pointer_tip, &tip_pose);

  demo_pointer_tip_update_apparent_size (self->pointer_tip, self->context);

  demo_pointer_tip_set_active (self->pointer_tip, FALSE);
}

static void
_controller_move_cb (GxrController *controller,
                     GxrQuitEvent *event,
                     SceneController *self)
{
  (void) event;
  (void) self;

  graphene_matrix_t pose;
  gxr_controller_get_pointer_pose (controller, &pose);

  /* when hovering or grabbing, the tip's transform is controlled by
   * gxr_controller_update_hovered_object() because the tip is oriented
   * towards the surface it is on, which we have not stored in hover_state. */
  if (self->hover_state.hovered_object == NULL &&
    self->grab_state.grabbed_object == NULL)
    _no_hover_transform_tip (self, &pose);

  /* The pointer's pose is always set by pose update. When hovering, only the
   * length is set by gxr_controller_update_hovered_object(). */
  demo_pointer_move (self->pointer_ray, &pose);
}

void
scene_controller_hide_pointer (SceneController *self)
{
  demo_pointer_hide (self->pointer_ray);
  demo_pointer_tip_hide (self->pointer_tip);
}

void
scene_controller_show_pointer (SceneController *self)
{
  demo_pointer_show (self->pointer_ray);
  demo_pointer_tip_show (self->pointer_tip);
}

gboolean
scene_controller_is_pointer_visible (SceneController *self)
{
  return demo_pointer_tip_is_visible (self->pointer_tip);
}

void
scene_controller_update_hovered_object (SceneController *self,
                                      gpointer last_object,
                                      gpointer object,
                                      graphene_matrix_t *object_pose,
                                      graphene_point3d_t *intersection_point,
                                      graphene_point_t *intersection_2d,
                                      float intersection_distance)
{
  if (last_object != object)
    demo_pointer_tip_set_active (self->pointer_tip, object != NULL);

  if (object)
    {
      self->hover_state.hovered_object = object;

      graphene_point_init_from_point (&self->hover_state.intersection_2d,
                                      intersection_2d);

      self->hover_state.distance = intersection_distance;

      demo_pointer_tip_update (self->pointer_tip, self->context,
                              object_pose, intersection_point);

      demo_pointer_set_length (self->pointer_ray, intersection_distance);
    }
  else
    {
      graphene_matrix_t pointer_pose;
      gxr_controller_get_pointer_pose (GXR_CONTROLLER (self->controller),
                                       &pointer_pose);

      graphene_quaternion_t controller_rotation;
      graphene_quaternion_init_from_matrix (&controller_rotation,
                                            &pointer_pose);

      graphene_point3d_t distance_point;
      graphene_point3d_init (&distance_point,
                            0.f,
                            0.f,
                            -demo_pointer_get_default_length (self->pointer_ray));

      graphene_point3d_t controller_position;
      graphene_ext_matrix_get_translation_point3d (&pointer_pose,
                                                   &controller_position);

      graphene_matrix_t tip_pose;
      graphene_matrix_init_identity (&tip_pose);
      graphene_matrix_translate (&tip_pose, &distance_point);
      graphene_matrix_rotate_quaternion (&tip_pose, &controller_rotation);
      graphene_matrix_translate (&tip_pose, &controller_position);
      demo_pointer_tip_set_transformation (self->pointer_tip, &tip_pose);
      demo_pointer_tip_update_apparent_size (self->pointer_tip, self->context);


      if (last_object)
        {
          demo_pointer_reset_length (self->pointer_ray);
          scene_controller_reset_hover_state (self);
          scene_controller_reset_grab_state (self);
        }
    }

  /* TODO:
  demo_pointer_set_selected_object (self->pointer_ray, object);
  */
}

void
scene_controller_drag_start (SceneController *self,
                           gpointer grabbed_object,
                           graphene_matrix_t *object_pose)
{
  self->grab_state.grabbed_object = grabbed_object;

  graphene_matrix_t pointer_pose;
  gxr_controller_get_pointer_pose (GXR_CONTROLLER (self->controller),
                                   &pointer_pose);

  graphene_quaternion_t controller_rotation;
  graphene_ext_matrix_get_rotation_quaternion (&pointer_pose,
                                               &controller_rotation);

  graphene_ext_matrix_get_rotation_quaternion (object_pose,
                                               &self->grab_state.object_rotation);

  graphene_point_init (
    &self->grab_state.grab_offset,
    -self->hover_state.intersection_2d.x,
    -self->hover_state.intersection_2d.y);

  graphene_quaternion_invert (
    &controller_rotation,
    &self->grab_state.inverse_controller_rotation);
}

gboolean
scene_controller_get_drag_pose (SceneController     *self,
                              graphene_matrix_t *drag_pose)
{
  if (self->grab_state.grabbed_object == NULL)
    return FALSE;

  graphene_matrix_t pointer_pose;
  gxr_controller_get_pointer_pose (GXR_CONTROLLER (self->controller),
                                   &pointer_pose);

  graphene_point3d_t controller_translation_point;
  graphene_ext_matrix_get_translation_point3d (&pointer_pose,
                                               &controller_translation_point);
  graphene_quaternion_t controller_rotation;
  graphene_quaternion_init_from_matrix (&controller_rotation,
                                        &pointer_pose);

  graphene_point3d_t distance_translation_point;
  graphene_point3d_init (&distance_translation_point,
                         0.f, 0.f, -self->hover_state.distance);

  /* Build a new transform for pointer tip in event->pose.
   * Pointer tip is at intersection, in the plane of the window,
   * so we can reuse the tip rotation for the window rotation. */
  graphene_matrix_init_identity (&self->intersection_pose);

  /* restore original rotation of the tip */
  graphene_matrix_rotate_quaternion (&self->intersection_pose,
                                     &self->grab_state.object_rotation);

  /* Later the current controller rotation is applied to the overlay, so to
   * keep the later controller rotations relative to the initial controller
   * rotation, rotate the window in the opposite direction of the initial
   * controller rotation.
   * This will initially result in the same window rotation so the window does
   * not change its rotation when being grabbed, and changing the controllers
   * position later will rotate the window with the "diff" of the controller
   * rotation to the initial controller rotation. */
  graphene_matrix_rotate_quaternion (
    &self->intersection_pose, &self->grab_state.inverse_controller_rotation);

  /* then translate the overlay to the controller ray distance */
  graphene_matrix_translate (&self->intersection_pose,
                             &distance_translation_point);

  /* Rotate the translated overlay to where the controller is pointing. */
  graphene_matrix_rotate_quaternion (&self->intersection_pose,
                                     &controller_rotation);

  /* Calculation was done for controller in (0,0,0), just move it with
   * controller's offset to real (0,0,0) */
  graphene_matrix_translate (&self->intersection_pose,
                             &controller_translation_point);



  graphene_matrix_t transformation_matrix;
  graphene_matrix_init_identity (&transformation_matrix);

  graphene_point3d_t grab_offset3d;
  graphene_point3d_init (&grab_offset3d,
                         self->grab_state.grab_offset.x,
                         self->grab_state.grab_offset.y,
                         0);

  /* translate such that the grab point is pivot point. */
  graphene_matrix_translate (&transformation_matrix, &grab_offset3d);

  /* window has the same rotation as the tip we calculated in event->pose */
  graphene_matrix_multiply (&transformation_matrix,
                            &self->intersection_pose,
                            &transformation_matrix);

  graphene_matrix_init_from_matrix (drag_pose, &transformation_matrix);

  /* TODO:
  XrdPointer *pointer = self->pointer_ray;
  xrd_pointer_set_selected_window (pointer, window);
  */

  demo_pointer_tip_set_transformation (self->pointer_tip,
                                      &self->intersection_pose);
  /* update apparent size after pointer has been moved */
  demo_pointer_tip_update_apparent_size (self->pointer_tip, self->context);

  return TRUE;
}

void
scene_controller_set_user_data (SceneController *self, gpointer data)
{
  self->user_data = data;
}

gpointer
scene_controller_get_user_data (SceneController *self)
{
  return self->user_data;
}

GxrController *
scene_controller_get_controller (SceneController *self)
{
  return self->controller;
}
