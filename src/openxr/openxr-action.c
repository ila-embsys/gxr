/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Christoph Haag <christop.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gdk/gdk.h>

#include "gxr-types.h"
#include "gxr-io.h"

#include "openxr-action.h"
#include "openxr-context.h"
#include "openxr-action-set.h"
#include "openxr-action-set-private.h"
#include "openxr-action-binding.h"

#define NUM_HANDS 2
struct _OpenXRAction
{
  GxrAction parent;

  XrInstance instance;
  XrSession session;

  XrPath hand_paths[NUM_HANDS];

  /* only used when this action is a pose action*/
  XrSpace hand_spaces[NUM_HANDS];
  XrSpace tracked_space;

  char *url;

  XrAction handle;
};

G_DEFINE_TYPE (OpenXRAction, openxr_action, GXR_TYPE_ACTION)

gboolean
openxr_action_load_manifest (char *path)
{
  (void) path;
  return TRUE;
}

static void
openxr_action_init (OpenXRAction *self)
{
  self->handle = XR_NULL_HANDLE;
  for (int i = 0; i < NUM_HANDS; i++)
    self->hand_spaces[i] = XR_NULL_HANDLE;

  /* TODO: Handle this more nicely */
  OpenXRContext *context = OPENXR_CONTEXT (gxr_context_get_instance ());
  self->instance = openxr_context_get_openxr_instance (context);
  self->session = openxr_context_get_openxr_session (context);
  self->tracked_space = openxr_context_get_tracked_space (context);

  xrStringToPath(self->instance, "/user/hand/left", &self->hand_paths[0]);
  xrStringToPath(self->instance, "/user/hand/right", &self->hand_paths[1]);
}

OpenXRAction *
openxr_action_new (void)
{
  return (OpenXRAction*) g_object_new (OPENXR_TYPE_ACTION, 0);
}

static gboolean
_url_to_name (char *url, char *name)
{
  char *basename = g_path_get_basename (url);
  if (g_strcmp0 (basename, ".") == 0)
    return false;

  strncpy (name, basename, XR_MAX_ACTION_NAME_SIZE);
  return true;
}

OpenXRAction *
openxr_action_new_from_type_url (GxrActionSet *action_set,
                                 GxrActionType type, char *url)
{
  OpenXRAction *self = openxr_action_new ();
  gxr_action_set_action_type (GXR_ACTION (self), type);
  gxr_action_set_url (GXR_ACTION (self), g_strdup (url));
  gxr_action_set_action_set(GXR_ACTION (self), action_set);
  self->url = g_strdup (url);

  XrActionType action_type;
  switch (type)
  {
    case GXR_ACTION_ANALOG:
      action_type = XR_ACTION_TYPE_VECTOR2F_INPUT;
      break;
    case GXR_ACTION_DIGITAL:
      action_type = XR_ACTION_TYPE_BOOLEAN_INPUT;
      break;
    case GXR_ACTION_POSE:
      action_type = XR_ACTION_TYPE_POSE_INPUT;
      break;
    default:
      g_printerr ("Unknown action type %d\n", type);
      action_type = XR_ACTION_TYPE_BOOLEAN_INPUT;
  }

  XrActionCreateInfo action_info = {
    .type = XR_TYPE_ACTION_CREATE_INFO,
    .next = NULL,
    .actionType = action_type,
    .countSubactionPaths = NUM_HANDS,
    .subactionPaths = self->hand_paths
  };

  /* TODO: proper names, localized name */
  /* TODO: proper names, localized name */
  char name[XR_MAX_ACTION_NAME_SIZE];
  _url_to_name (self->url, name);

  strcpy(action_info.actionName, name);
  strcpy(action_info.localizedActionName, name);

  XrActionSet set = openxr_action_set_get_handle (OPENXR_ACTION_SET (action_set));

  XrResult result = xrCreateAction (set, &action_info, &self->handle);


  if (result != XR_SUCCESS)
    {
      char buffer[XR_MAX_RESULT_STRING_SIZE];
      xrResultToString(self->instance, result, buffer);
      g_printerr ("Failed to create action %s: %s\n", url, buffer);
      g_object_unref (self);
      self = NULL;
    }

  if (action_type == XR_ACTION_TYPE_POSE_INPUT)
    {
      for (int i = 0; i < NUM_HANDS; i++)
        {
          XrActionSpaceCreateInfo action_space_info = {
            .type = XR_TYPE_ACTION_SPACE_CREATE_INFO,
            .next = NULL,
            .action = self->handle,
            .poseInActionSpace.orientation.w = 1.f,
            .subactionPath = self->hand_paths[i]
          };

          result = xrCreateActionSpace(self->session,
                                       &action_space_info,
                                       &self->hand_spaces[i]);

          if (result != XR_SUCCESS)
            {
              char buffer[XR_MAX_RESULT_STRING_SIZE];
              xrResultToString(self->instance, result, buffer);
              g_printerr ("Failed to create action space %s: %s\n", url, buffer);
              g_object_unref (self);
              self = NULL;
            }
        }
    }
  return self;
}

static gboolean
_action_poll_digital (OpenXRAction *self)
{
  for (int i = 0; i < NUM_HANDS; i++)
    {
      XrActionStateGetInfo get_info = {
        .type = XR_TYPE_ACTION_STATE_GET_INFO,
        .next = NULL,
        .action = self->handle,
        .subactionPath = self->hand_paths[i]
      };

      XrActionStateBoolean value = {
        .type = XR_TYPE_ACTION_STATE_BOOLEAN,
        .next = NULL
      };

      XrResult result = xrGetActionStateBoolean(self->session, &get_info, &value);

      if (result != XR_SUCCESS)
        {
          g_debug ("Failed to poll digital action\n");
          continue;
        }

      GxrDigitalEvent *event = g_malloc (sizeof (GxrDigitalEvent));
      event->controller_handle = i;
      event->active = value.isActive;
      event->state = value.currentState;
      event->changed = value.changedSinceLastSync;
      event->time = value.lastChangeTime;

      gxr_action_emit_digital (GXR_ACTION (self), event);
    }

  return TRUE;
}

static gboolean
_action_poll_analog (OpenXRAction *self)
{

  for (int i = 0; i < NUM_HANDS; i++)
    {
      XrActionStateGetInfo get_info = {
        .type = XR_TYPE_ACTION_STATE_GET_INFO,
        .next = NULL,
        .action = self->handle,
        .subactionPath = self->hand_paths[i]
      };

      XrActionStateVector2f value = {
        .type = XR_TYPE_ACTION_STATE_VECTOR2F,
        .next = NULL
      };

      XrResult result = xrGetActionStateVector2f(self->session, &get_info, &value);

      if (result != XR_SUCCESS)
        {
          g_debug ("Failed to poll analog action\n");
          continue;
        }

      GxrAnalogEvent *event = g_malloc (sizeof (GxrAnalogEvent));
      event->controller_handle = i;
      event->active = value.isActive;
      graphene_vec3_init (&event->state, value.currentState.x, value.currentState.y, 0);
      graphene_vec3_init (&event->delta, 0, 0, 0); /* TODO */
      event->time = value.lastChangeTime;

      gxr_action_emit_analog (GXR_ACTION (self), event);
    }

  return TRUE;
}

static bool
_space_location_valid (XrSpaceLocation *sl)
{
  return
    /* (sl->locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)    != 0 && */
    (sl->locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0;
}

static void
_get_model_matrix_from_pose (XrPosef *pose,
                             graphene_matrix_t *mat)
{
  graphene_quaternion_t q;
  graphene_quaternion_init (&q,
                            pose->orientation.x, pose->orientation.y,
                            pose->orientation.z, pose->orientation.w);

  graphene_matrix_init_identity (mat);
  graphene_matrix_rotate_quaternion (mat, &q);
  graphene_point3d_t translation = { .x = pose->position.x, pose->position.y, pose->position.z };
  graphene_matrix_translate(mat, &translation);
}

static gboolean
_action_poll_pose_secs_from_now (OpenXRAction *self,
                                 float         secs)
{
  (void) secs;
  for (int i = 0; i < NUM_HANDS; i++)
    {
      XrActionStateGetInfo get_info = {
        .type = XR_TYPE_ACTION_STATE_GET_INFO,
        .next = NULL,
        .action = self->handle,
        .subactionPath = self->hand_paths[i]
      };

      XrActionStatePose value = {
        .type = XR_TYPE_ACTION_STATE_POSE,
        .next = NULL
      };

      XrResult result = xrGetActionStatePose (self->session, &get_info, &value);

      if (result != XR_SUCCESS)
        {
          g_debug ("Failed to poll analog action\n");
          continue;
        }

      bool spaceLocationValid;
      XrSpaceLocation space_location = {
        .type = XR_TYPE_SPACE_LOCATION,
        .next = NULL
      };

      /* TODO: need predicted display time here, not seconds from now */
      result = xrLocateSpace (self->hand_spaces[i], self->tracked_space, 0, &space_location);

      if (result != XR_SUCCESS)
        {
          g_debug ("Failed to poll hand space location\n");
          continue;
        }

      spaceLocationValid = _space_location_valid (&space_location);

      GxrPoseEvent *event = g_malloc (sizeof (GxrPoseEvent));
      event->active = value.isActive;
      event->controller_handle = i;
      _get_model_matrix_from_pose(&space_location.pose, &event->pose);
      graphene_vec3_init (&event->velocity, 0, 0, 0);
      graphene_vec3_init (&event->angular_velocity, 0, 0, 0);
      event->valid = spaceLocationValid;
      event->device_connected = true;

      gxr_action_emit_pose (GXR_ACTION (self), event);
    }

  return TRUE;
}

static gboolean
_poll (GxrAction *action)
{
  OpenXRAction *self = OPENXR_ACTION (action);
  GxrActionType type = gxr_action_get_action_type (action);
  switch (type)
    {
    case GXR_ACTION_DIGITAL:
      return _action_poll_digital (self);
    case GXR_ACTION_ANALOG:
      return _action_poll_analog (self);
    case GXR_ACTION_POSE:
      return _action_poll_pose_secs_from_now (self, 0);
    default:
      g_printerr ("Uknown action type %d\n", type);
      return FALSE;
    }
}

static gboolean
_trigger_haptic (GxrAction *action,
                 float start_seconds_from_now,
                 float duration_seconds,
                 float frequency,
                 float amplitude,
                 guint64 controller_handle)
{
  OpenXRAction *self = OPENXR_ACTION (action);
  (void) self;
  (void) start_seconds_from_now;
  (void) duration_seconds;
  (void) frequency;
  (void) amplitude;
  (void) controller_handle;

  g_print ("Stub: Trigger haptic\n");

  return TRUE;
}

char *
openxr_action_get_url (OpenXRAction *self)
{
  return self->url;
}

XrAction
openxr_action_get_handle (OpenXRAction *self)
{
  return self->handle;
}

static void
_finalize (GObject *gobject)
{
  OpenXRAction *self = OPENXR_ACTION (gobject);
  g_free (self->url);
}

static void
openxr_action_class_init (OpenXRActionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize =_finalize;

  GxrActionClass *gxr_action_class = GXR_ACTION_CLASS (klass);
  gxr_action_class->poll = _poll;
  gxr_action_class->trigger_haptic = _trigger_haptic;
}

gboolean
openxr_action_load_cached_manifest (const char* cache_name,
                                    const char* resource_path,
                                    const char* manifest_name,
                                    const char* first_binding,
                                    ...)
{
  /* Create cache directory if needed */
  GString* cache_path = gxr_io_get_cache_path (cache_name);

  if (g_mkdir_with_parents (cache_path->str, 0700) == -1)
    {
      g_printerr ("Unable to create directory %s\n", cache_path->str);
      return FALSE;
    }

  /* Cache actions manifest */
  GString *actions_path = g_string_new ("");
  if (!gxr_io_write_resource_to_file (resource_path,
                                      cache_path->str,
                                      manifest_name,
                                      actions_path))
    return FALSE;

  va_list args;

  const char* current = first_binding;

  va_start (args, first_binding);

  while (current != NULL)
    {
      GString *bindings_path = g_string_new ("");
      if (!gxr_io_write_resource_to_file (resource_path,
                                          cache_path->str,
                                          current,
                                          bindings_path))
        return FALSE;

      g_string_free (bindings_path, TRUE);

      current = va_arg (args, const char*);
    }

  va_end (args);

  g_string_free (cache_path, TRUE);

  if (!openxr_action_load_manifest (actions_path->str))
    return FALSE;

  g_string_free (actions_path, TRUE);

  return TRUE;
}
