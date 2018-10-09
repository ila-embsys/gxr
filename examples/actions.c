/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib-unix.h>

#include "openvr-context.h"
#include "openvr-io.h"
#include "openvr-action.h"
#include "openvr-action-set.h"

typedef struct Example
{
  GMainLoop *loop;
  OpenVRAction *haptic_primary;

  OpenVRActionSet *wm_action_set;
  OpenVRActionSet *mouse_synth_action_set;
} Example;

gboolean
_sigint_cb (gpointer _self)
{
  Example *self = (Example*) _self;
  g_main_loop_quit (self->loop);
  return TRUE;
}

static int haptics_counter = 0;

gboolean
_poll_events_cb (gpointer _self)
{
  Example *self = (Example*) _self;

  if (!openvr_action_set_poll (self->wm_action_set))
    return FALSE;

  if (!openvr_action_set_poll (self->mouse_synth_action_set))
    return FALSE;

  /* emit vibration every 200 polls */
  haptics_counter++;
  if (haptics_counter % 200 == 0)
    {
      if (!openvr_action_trigger_haptic (self->haptic_primary,
                                         0.0f ,1.0f, 4.0f, 1.0f))
        return FALSE;

      haptics_counter = 0;
    }

  return TRUE;
}

gboolean
_cache_bindings (GString *actions_path)
{
  GString* cache_path = openvr_io_get_cache_path ("openvr-glib");

  if (!openvr_io_create_directory_if_needed (cache_path->str))
    return FALSE;

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
             const gchar        *name)
{
  (void) action;

  g_print ("DIGITAL: %s: %d | %d\n", name, event->active, event->state);

  g_free (event);
}

static void
_analog_cb (OpenVRAction      *action,
            OpenVRAnalogEvent *event,
            const gchar       *name)
{
  (void) action;

  g_print ("ANALOG: %s: %d | %f %f %f | %f %f %f\n",
           name,
           event->active,
           graphene_vec3_get_x (&event->state),
           graphene_vec3_get_y (&event->state),
           graphene_vec3_get_z (&event->state),
           graphene_vec3_get_x (&event->delta),
           graphene_vec3_get_y (&event->delta),
           graphene_vec3_get_z (&event->delta));

  g_free (event);
}

static void
_pose_cb (OpenVRAction    *action,
          OpenVRPoseEvent *event,
          const gchar     *name)
{
  (void) action;

  g_print ("POSE: %s: %d | %f %f %f | %f %f %f\n",
           name,
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

void
_cleanup (Example *self)
{
  g_print ("bye\n");

  g_main_loop_unref (self->loop);

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  g_object_unref (self->wm_action_set);
  g_object_unref (self->mouse_synth_action_set);
  g_object_unref (self->haptic_primary);
}

int
main ()
{
  OpenVRContext *context = openvr_context_get_instance ();
  if (!openvr_context_init_overlay (context))
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
    .haptic_primary =
      openvr_action_new_from_url ("/actions/wm/out/haptic_primary")
  };

  openvr_action_set_connect (self.wm_action_set, OPENVR_ACTION_DIGITAL,
                             "/actions/wm/in/grab_window",
                             (GCallback) _digital_cb, "grab_window");

  openvr_action_set_connect (self.wm_action_set, OPENVR_ACTION_ANALOG,
                             "/actions/wm/in/push_pull",
                             (GCallback) _analog_cb, "push_pull");

  openvr_action_set_connect (self.wm_action_set, OPENVR_ACTION_POSE,
                             "/actions/wm/in/hand_primary",
                             (GCallback) _pose_cb, "hand_primary");

  openvr_action_set_connect (self.mouse_synth_action_set,
                             OPENVR_ACTION_DIGITAL,
                             "/actions/mouse_synth/in/left_click",
                             (GCallback) _digital_cb, "left_click");

  g_timeout_add (20, _poll_events_cb, &self);

  g_unix_signal_add (SIGINT, _sigint_cb, &self);

  g_main_loop_run (self.loop);

  _cleanup (&self);

  return 0;
}
