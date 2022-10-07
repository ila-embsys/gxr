/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-action-set.h"
#include "gxr-action.h"
#include "gxr-context-private.h"
#include "gxr-manifest.h"

struct _GxrActionSet
{
  GObject parent;

  GSList *actions;

  GxrContext *context;
  char       *url;

  GxrManifest *manifest;

  XrActionSet handle;
};

G_DEFINE_TYPE (GxrActionSet, gxr_action_set, G_TYPE_OBJECT)

static void
gxr_action_set_finalize (GObject *gobject);

static void
gxr_action_set_class_init (GxrActionSetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gxr_action_set_finalize;
}

static void
gxr_action_set_init (GxrActionSet *self)
{
  self->actions = NULL;
  self->handle = XR_NULL_HANDLE;
  self->manifest = NULL;
}

static gboolean
_url_to_name (char *url, char *name)
{
  char *basename = g_path_get_basename (url);
  if (g_strcmp0 (basename, ".") == 0)
    {
      g_free (basename);
      return FALSE;
    }

  strncpy (name, basename, XR_MAX_ACTION_NAME_SIZE - 1);
  g_free (basename);
  return TRUE;
}

static void
_printerr_xr_result (XrInstance instance, XrResult result)
{
  char buffer[XR_MAX_RESULT_STRING_SIZE];
  xrResultToString (instance, result, buffer);
  g_printerr ("%s\n", buffer);
}

GxrActionSet *
gxr_action_set_new_from_url (GxrContext  *context,
                             GxrManifest *manifest,
                             gchar       *url)
{
  GxrActionSet *self = (GxrActionSet *) g_object_new (GXR_TYPE_ACTION_SET, 0);

  self->context = context;
  self->manifest = g_object_ref (manifest);
  self->url = g_strdup (url);

  XrActionSetCreateInfo set_info = {
    .type = XR_TYPE_ACTION_SET_CREATE_INFO,
    .next = NULL,
    .priority = 0,
  };

  /* TODO: proper names, localized name */
  char name[XR_MAX_ACTION_NAME_SIZE];
  _url_to_name (self->url, name);

  strcpy (set_info.actionSetName, name);
  strcpy (set_info.localizedActionSetName, name);

  XrInstance instance = gxr_context_get_openxr_instance (context);

  XrResult result = xrCreateActionSet (instance, &set_info, &self->handle);

  if (result != XR_SUCCESS)
    {
      g_printerr ("Failed to create action set: ");
      _printerr_xr_result (instance, result);
      g_clear_object (&self);
    }

  return self;
}

static void
gxr_action_set_finalize (GObject *gobject)
{
  GxrActionSet *self = GXR_ACTION_SET (gobject);
  g_slist_free_full (self->actions, g_object_unref);
  g_free (self->url);
  g_clear_object (&self->manifest);
  G_OBJECT_CLASS (gxr_action_set_parent_class)->finalize (gobject);
}

static gboolean
_update (GxrActionSet **sets, uint32_t count)
{
  if (count == 0)
    return TRUE;

  /* All actionsets must be attached to the same session */
  XrInstance instance = gxr_context_get_openxr_instance (sets[0]->context);
  XrSession  session = gxr_context_get_openxr_session (sets[0]->context);

  XrSessionState state = gxr_context_get_session_state (sets[0]->context);
  /* just pretend no input happens when we're not focused */
  if (state != XR_SESSION_STATE_FOCUSED)
    return TRUE;

  XrActiveActionSet *active_action_sets = g_malloc (sizeof (XrActiveActionSet)
                                                    * count);
  for (uint32_t i = 0; i < count; i++)
    {
      GxrActionSet *self = sets[i];
      active_action_sets[i].actionSet = self->handle;
      active_action_sets[i].subactionPath = XR_NULL_PATH;
    }

  XrActionsSyncInfo syncInfo = {
    .type = XR_TYPE_ACTIONS_SYNC_INFO,
    .countActiveActionSets = count,
    .activeActionSets = active_action_sets,
  };

  XrResult result = xrSyncActions (session, &syncInfo);

  g_free (active_action_sets);

  if (result == XR_SESSION_NOT_FOCUSED)
    {
      /* xrSyncActions can be called before reading the session state change */
      g_debug ("SyncActions while session not focused");
      return TRUE;
    }

  if (result != XR_SUCCESS)
    {
      g_printerr ("ERROR: SyncActions: ");
      _printerr_xr_result (instance, result);
      return FALSE;
    }

  return TRUE;
}

gboolean
gxr_action_sets_poll (GxrActionSet **sets, uint32_t count)
{
  if (!_update (sets, count))
    return FALSE;

  for (uint32_t i = 0; i < count; i++)
    {
      for (GSList *l = sets[i]->actions; l != NULL; l = l->next)
        {
          GxrAction *action = (GxrAction *) l->data;

          /* haptic has no inputs, can't be polled */
          if (gxr_action_get_action_type (action) == GXR_ACTION_HAPTIC)
            continue;

          if (!gxr_action_poll (action))
            return FALSE;
        }
    }
  return TRUE;
}

GxrAction *
gxr_action_set_connect_digital_from_float (GxrActionSet *self,
                                           GxrContext   *context,
                                           gchar        *url,
                                           float         threshold,
                                           char         *haptic_url,
                                           GCallback     callback,
                                           gpointer      data)
{
  GxrAction *action
    = gxr_action_new_from_type_url (context, self,
                                    GXR_ACTION_DIGITAL_FROM_FLOAT, url);

  if (action != NULL)
    self->actions = g_slist_append (self->actions, action);

  GxrAction *haptic_action = NULL;
  if (haptic_url)
    {
      haptic_action = gxr_action_new_from_type_url (context, self,
                                                    GXR_ACTION_HAPTIC,
                                                    haptic_url);
      if (haptic_action != NULL)
        {
          self->actions = g_slist_append (self->actions, haptic_action);
          gxr_action_set_digital_from_float_haptic (action, haptic_action);
        }
    }
  gxr_action_set_digital_from_float_threshold (action, threshold);

  g_signal_connect (action, "digital-event", callback, data);

  return action;
}

gboolean
gxr_action_set_connect (GxrActionSet *self,
                        GxrContext   *context,
                        GxrActionType type,
                        gchar        *url,
                        GCallback     callback,
                        gpointer      data)
{
  GxrAction *action = gxr_action_new_from_type_url (context, self, type, url);

  if (action != NULL)
    self->actions = g_slist_append (self->actions, action);
  else
    {
      g_printerr ("Failed to create/connect action %s\n", url);
      return FALSE;
    }

  switch (type)
    {
      case GXR_ACTION_DIGITAL:
        g_signal_connect (action, "digital-event", callback, data);
        break;
      case GXR_ACTION_FLOAT:
      case GXR_ACTION_VEC2F:
        g_signal_connect (action, "analog-event", callback, data);
        break;
      case GXR_ACTION_POSE:
        g_signal_connect (action, "pose-event", callback, data);
        break;
      case GXR_ACTION_HAPTIC:
        /* no input, only output */
        break;
      default:
        g_printerr ("Unknown action type %d\n", type);
        return FALSE;
    }

  return TRUE;
}

GSList *
gxr_action_set_get_actions (GxrActionSet *self)
{
  return self->actions;
}

XrActionSet
gxr_action_set_get_handle (GxrActionSet *self)
{
  return self->handle;
}

GxrManifest *
gxr_action_set_get_manifest (GxrActionSet *self)
{
  return self->manifest;
}
