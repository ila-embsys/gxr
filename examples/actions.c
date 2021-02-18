/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib-unix.h>

#include "gxr.h"

enum {
  WM_ACTIONSET,
  SYNTH_ACTIONSET,
  LAST_ACTIONSET
};

typedef struct Example
{
  GMainLoop *loop;
  GxrAction *haptic;
  GxrContext *context;

  /* array of action sets */
  GxrActionSet *action_sets[LAST_ACTIONSET];
} Example;

static gboolean
_sigint_cb (gpointer _self)
{
  Example *self = (Example*) _self;
  g_main_loop_quit (self->loop);
  return TRUE;
}

static gboolean
_poll_events_cb (gpointer _self)
{
  Example *self = (Example*) _self;

  if (!gxr_action_sets_poll (self->action_sets, 2))
    return FALSE;

  return TRUE;
}

static void
_digital_cb (GxrAction       *action,
             GxrDigitalEvent *event,
             Example         *self)
{
  (void) action;
  (void) self;

  g_print ("DIGITAL (%lu): %d | %d\n",
           gxr_device_get_handle (GXR_DEVICE (event->controller)),
           event->active, event->state);

  if (event->changed)
    {
      gxr_action_trigger_haptic (self->haptic, 0.0f ,.2f, 160.f, 1.0f,
                                 gxr_device_get_handle (GXR_DEVICE (event->controller)));
    }

  g_free (event);
}
#include <stdio.h>
static void
_hand_pose_cb (GxrAction    *action,
               GxrPoseEvent *event,
               Example      *self)
{
  (void) action;
  (void) self;

  g_print ("POSE (%lu): %d | %f %f %f | %f %f %f\n",
           gxr_device_get_handle (GXR_DEVICE (event->controller)),
           event->active,
           graphene_vec3_get_x (&event->velocity),
           graphene_vec3_get_y (&event->velocity),
           graphene_vec3_get_z (&event->velocity),
           graphene_vec3_get_x (&event->angular_velocity),
           graphene_vec3_get_y (&event->angular_velocity),
           graphene_vec3_get_z (&event->angular_velocity));

  graphene_matrix_print (&event->pose);

  g_free (event);
}

static void
_cleanup (Example *self)
{
  g_print ("bye\n");

  g_main_loop_unref (self->loop);
  g_object_unref (self->context);

  for (int i = 0; i < 2; i++)
    g_object_unref (self->action_sets[i]);
  g_object_unref (self->haptic);
}

int
main ()
{
  Example self = {
    .loop = g_main_loop_new (NULL, FALSE),
    .context = gxr_context_new (GXR_APP_SCENE, "Actions Example", 1)
  };

  if (!gxr_context_load_action_manifest (
        self.context,
        "gxr",
        "/res/bindings",
        "actions.json",
        "bindings_vive_controller.json",
        "bindings_knuckles_controller.json",
        NULL))
    {
      g_print ("Failed to load action bindings!\n");
      return 1;
    }

  self.action_sets[WM_ACTIONSET] =
    gxr_action_set_new_from_url (self.context, "/actions/wm");
  self.action_sets[SYNTH_ACTIONSET] =
    gxr_action_set_new_from_url (self.context, "/actions/mouse_synth");

  self.haptic =
    gxr_action_new_from_type_url (self.context,
                                  self.action_sets[WM_ACTIONSET],
                                  GXR_ACTION_HAPTIC,
                                  "/actions/wm/out/haptic");

  gxr_action_set_connect (self.action_sets[WM_ACTIONSET], self.context,
                          GXR_ACTION_POSE,
                          "/actions/wm/in/hand_pose",
                          (GCallback) _hand_pose_cb, &self);

  gxr_action_set_connect (self.action_sets[SYNTH_ACTIONSET], self.context,
                          GXR_ACTION_DIGITAL,
                          "/actions/mouse_synth/in/left_click",
                          (GCallback) _digital_cb, &self);

  g_timeout_add (20, _poll_events_cb, &self);

  g_unix_signal_add (SIGINT, _sigint_cb, &self);

  g_main_loop_run (self.loop);

  _cleanup (&self);

  return 0;
}
