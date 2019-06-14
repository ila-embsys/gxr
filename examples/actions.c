/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib-unix.h>

#include "openvr-glib.h"

typedef struct Example
{
  GMainLoop *loop;
  OpenVRAction *haptic_left;
  OpenVRAction *haptic_right;

  OpenVRActionSet *wm_action_set;
  OpenVRActionSet *mouse_synth_action_set;
} Example;

typedef struct ActionCallbackData
{
  Example     *self;
  int         controller_index;
} ActionCallbackData;

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

  if (!openvr_action_set_poll (self->wm_action_set))
    return FALSE;

  if (!openvr_action_set_poll (self->mouse_synth_action_set))
    return FALSE;

  return TRUE;
}

static gboolean
_cache_bindings (GString *actions_path)
{
  GString* cache_path = openvr_io_get_cache_path ("openvr-glib");

  if (g_mkdir_with_parents (cache_path->str, 0700) == -1)
    {
      g_printerr ("Unable to create directory %s\n", cache_path->str);
      return FALSE;
    }

  if (!openvr_io_write_resource_to_file ("/res/bindings", cache_path->str,
                                         "actions.json", actions_path))
    return FALSE;

  GString *bindings_path = g_string_new ("");
  if (!openvr_io_write_resource_to_file ("/res/bindings", cache_path->str,
                                         "bindings_vive_controller.json",
                                         bindings_path))
    return FALSE;

  g_string_free (bindings_path, TRUE);
  g_string_free (cache_path, TRUE);

  return TRUE;
}

static void
_digital_cb (OpenVRAction       *action,
             OpenVRDigitalEvent *event,
             ActionCallbackData *data)
{
  (void) action;

  g_print ("DIGITAL (%d): %d | %d\n",
           data->controller_index, event->active, event->state);

  if (event->changed)
    {
      openvr_action_trigger_haptic (
          (data->controller_index == 0 ?
           data->self->haptic_left : data->self->haptic_right),
           0.0f ,1.0f, 4.0f, 1.0f);
    }
  g_free (event);
}
#include <stdio.h>
static void
_hand_pose_cb (OpenVRAction       *action,
               OpenVRPoseEvent    *event,
               ActionCallbackData *data)
{
  (void) action;

  g_print ("POSE (%d): %d | %f %f %f | %f %f %f\n",
           data->controller_index,
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

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  g_object_unref (self->wm_action_set);
  g_object_unref (self->mouse_synth_action_set);
  g_object_unref (self->haptic_left);
  g_object_unref (self->haptic_right);
}

int
main ()
{
  OpenVRContext *context = openvr_context_get_instance ();
  if (!openvr_context_initialize (context, OPENVR_APP_OVERLAY))
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  GString *action_manifest_path = g_string_new ("");
  if (!_cache_bindings (action_manifest_path))
    return FALSE;

  if (!openvr_action_load_manifest (action_manifest_path->str))
    return FALSE;

  g_string_free (action_manifest_path, TRUE);

  Example self = {
    .loop = g_main_loop_new (NULL, FALSE),
    .wm_action_set = openvr_action_set_new_from_url ("/actions/wm"),
    .mouse_synth_action_set =
      openvr_action_set_new_from_url ("/actions/mouse_synth"),
  };

  self.haptic_left =
    openvr_action_new_from_url (self.wm_action_set,
                                "/actions/wm/out/haptic_left");
  self.haptic_right =
    openvr_action_new_from_url (self.wm_action_set,
                                "/actions/wm/out/haptic_right");

  ActionCallbackData data_left =
    {
      .self = &self,
      .controller_index = 0
    };
  ActionCallbackData data_right =
    {
      .self = &self,
      .controller_index = 1
    };

  openvr_action_set_connect (self.wm_action_set, OPENVR_ACTION_POSE,
                             "/actions/wm/in/hand_pose_left",
                             (GCallback) _hand_pose_cb, &data_left);
  openvr_action_set_connect (self.wm_action_set, OPENVR_ACTION_POSE,
                             "/actions/wm/in/hand_pose_right",
                             (GCallback) _hand_pose_cb, &data_right);

  openvr_action_set_connect (self.mouse_synth_action_set,
                             OPENVR_ACTION_DIGITAL,
                             "/actions/mouse_synth/in/left_click_left",
                             (GCallback) _digital_cb, &data_left);
  openvr_action_set_connect (self.mouse_synth_action_set,
                             OPENVR_ACTION_DIGITAL,
                             "/actions/mouse_synth/in/left_click_right",
                             (GCallback) _digital_cb, &data_right);

  g_timeout_add (20, _poll_events_cb, &self);

  g_unix_signal_add (SIGINT, _sigint_cb, &self);

  g_main_loop_run (self.loop);

  _cleanup (&self);

  return 0;
}
