/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib-unix.h>
#include <gxr.h>

enum
{
  WM_ACTIONSET,
  SYNTH_ACTIONSET,
  LAST_ACTIONSET
};

typedef struct Example
{
  GMainLoop   *loop;
  GxrAction   *haptic;
  GxrContext  *context;
  GxrManifest *manifest;

  /* array of action sets */
  GxrActionSet *action_sets[LAST_ACTIONSET];
} Example;

static gboolean
_sigint_cb (gpointer _self)
{
  Example *self = (Example *) _self;
  g_main_loop_quit (self->loop);
  return TRUE;
}

static gboolean
_poll_events_cb (gpointer _self)
{
  Example *self = (Example *) _self;

  if (!gxr_context_begin_frame (self->context))
    return FALSE;

  // nothing rendered, depth values are irrelevant
  gxr_context_end_frame (self->context, 0.1f, 1.0f, 0.0f, 1.0f);
  gxr_context_poll_events (self->context);

  if (!gxr_action_sets_poll (self->action_sets, 2))
    {
      g_print ("Failed to poll events\n");
      return FALSE;
    }

  return TRUE;
}

static void
_digital_cb (GxrAction *action, GxrDigitalEvent *event, Example *self)
{
  (void) action;
  (void) self;

  g_print ("DIGITAL (%lu): %d | %d\n",
           gxr_device_get_handle (GXR_DEVICE (event->controller)),
           event->active, event->state);

  if (event->changed)
    {
      gxr_action_trigger_haptic (
        self->haptic, 0.0f, .2f, 160.f, 1.0f,
        gxr_device_get_handle (GXR_DEVICE (event->controller)));
    }
}

static void
_hand_pose_cb (GxrAction *action, GxrPoseEvent *event, Example *self)
{
  (void) action;
  (void) self;

  g_print ("POSE (%lu): %d | %f %f %f | %f %f %f\n",
           gxr_device_get_handle (GXR_DEVICE (event->controller)),
           event->active, graphene_vec3_get_x (&event->velocity),
           graphene_vec3_get_y (&event->velocity),
           graphene_vec3_get_z (&event->velocity),
           graphene_vec3_get_x (&event->angular_velocity),
           graphene_vec3_get_y (&event->angular_velocity),
           graphene_vec3_get_z (&event->angular_velocity));

  graphene_matrix_print (&event->pose);
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
    .context = gxr_context_new ("Actions Example", 1),
  };

  self.manifest = gxr_manifest_new ("/res/bindings", "actions.json");

  if (!self.manifest)
    {
      g_print ("Failed to load action bindings!\n");
      return 1;
    }

  self.action_sets[WM_ACTIONSET] = gxr_action_set_new_from_url (self.context,
                                                                self.manifest,
                                                                "/actions/wm");
  self.action_sets[SYNTH_ACTIONSET]
    = gxr_action_set_new_from_url (self.context, self.manifest,
                                   "/actions/mouse_synth");

  self.haptic = gxr_action_new_from_type_url (self.context,
                                              self.action_sets[WM_ACTIONSET],
                                              GXR_ACTION_HAPTIC,
                                              "/actions/wm/out/haptic");

  gxr_action_set_append_action (self.action_sets[WM_ACTIONSET], self.haptic);

  gxr_action_set_connect (self.action_sets[WM_ACTIONSET], GXR_ACTION_POSE,
                          "/actions/wm/in/hand_pose", (GCallback) _hand_pose_cb,
                          &self);

  gxr_action_set_connect (self.action_sets[SYNTH_ACTIONSET], GXR_ACTION_DIGITAL,
                          "/actions/mouse_synth/in/left_click",
                          (GCallback) _digital_cb, &self);

  gxr_context_attach_action_sets (self.context, self.action_sets, 2);

  g_timeout_add (20, _poll_events_cb, &self);

  g_unix_signal_add (SIGINT, _sigint_cb, &self);

  g_main_loop_run (self.loop);

  _cleanup (&self);

  return 0;
}
