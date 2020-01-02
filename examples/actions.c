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
           event->controller_handle, event->active, event->state);

  if (event->changed)
    {
      gxr_action_trigger_haptic (self->haptic, 0.0f ,.2f, 160.f, 1.0f,
                                 event->controller_handle);
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
           event->controller_handle,
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

  GxrContext *context = gxr_context_get_instance ();
  g_object_unref (context);

  for (int i = 0; i < 2; i++)
    g_object_unref (self->action_sets[i]);
  g_object_unref (self->haptic);
}

int
main ()
{
  GxrContext *context = gxr_context_get_instance ();
  if (!gxr_context_init_runtime (context, GXR_APP_OVERLAY))
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  if (!openvr_action_load_cached_manifest (
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

  Example self = {
    .loop = g_main_loop_new (NULL, FALSE),
  };

  self.action_sets[WM_ACTIONSET] = (GxrActionSet*)
    openvr_action_set_new_from_url ("/actions/wm");
  self.action_sets[SYNTH_ACTIONSET] = (GxrActionSet*)
    openvr_action_set_new_from_url ("/actions/mouse_synth");

  self.haptic = (GxrAction*)
    openvr_action_new_from_type_url (self.action_sets[WM_ACTIONSET],
                                     GXR_ACTION_HAPTIC,
                                     "/actions/wm/out/haptic");

  gxr_action_set_connect (self.action_sets[WM_ACTIONSET], GXR_ACTION_POSE,
                          "/actions/wm/in/hand_pose",
                          (GCallback) _hand_pose_cb, &self);

  gxr_action_set_connect (self.action_sets[SYNTH_ACTIONSET],
                          GXR_ACTION_DIGITAL,
                          "/actions/mouse_synth/in/left_click",
                          (GCallback) _digital_cb, &self);

  g_timeout_add (20, _poll_events_cb, &self);

  g_unix_signal_add (SIGINT, _sigint_cb, &self);

  g_main_loop_run (self.loop);

  _cleanup (&self);

  return 0;
}
