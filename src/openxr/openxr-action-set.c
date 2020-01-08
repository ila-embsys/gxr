/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Christoph Haag <christop.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openxr-action-set.h"
#include "openxr-action-set-private.h"

#include "openxr-action.h"

#include "openxr-context.h"

#include <openxr/openxr.h>

#include "openxr-action-binding.h"


struct _OpenXRActionSet
{
  GxrActionSet parent;

  XrInstance instance;
  XrSession session;

  char *url;

  XrActionSet handle;
};

G_DEFINE_TYPE (OpenXRActionSet, openxr_action_set, GXR_TYPE_ACTION_SET)

XrActionSet
openxr_action_set_get_handle (OpenXRActionSet *self)
{
  return self->handle;
}

static void
openxr_action_set_finalize (GObject *gobject);

static gboolean
_update (GxrActionSet **sets, uint32_t count);

static void
openxr_action_set_init (OpenXRActionSet *self)
{
  self->handle = XR_NULL_HANDLE;

  /* TODO: Handle this more nicely */
  OpenXRContext *context = OPENXR_CONTEXT (gxr_context_get_instance ());
  self->instance = openxr_context_get_openxr_instance (context);
  self->session = openxr_context_get_openxr_session (context);
}

OpenXRActionSet *
openxr_action_set_new (void)
{
  return (OpenXRActionSet*) g_object_new (OPENXR_TYPE_ACTION_SET, 0);
}

static void
_printerr_xr_result (XrInstance instance, XrResult result)
{
  char buffer[XR_MAX_RESULT_STRING_SIZE];
  xrResultToString(instance, result, buffer);
  g_printerr ("%s\n", buffer);
}

static gboolean
_url_to_name (char *url, char *name)
{
  char *basename = g_path_get_basename (url);
  if (g_strcmp0 (basename, ".") == 0)
    return false;

  strncpy (name, basename, XR_MAX_ACTION_NAME_SIZE - 1);
  return true;
}

OpenXRActionSet *
openxr_action_set_new_from_url (gchar *url)
{
  OpenXRActionSet *self = openxr_action_set_new ();
  self->url = g_strdup (url);

  XrActionSetCreateInfo set_info = {
    .type = XR_TYPE_ACTION_SET_CREATE_INFO,
    .next = NULL,
    .priority = 0
  };

  /* TODO: proper names, localized name */
  char name[XR_MAX_ACTION_NAME_SIZE];
  _url_to_name (self->url, name);

  strcpy(set_info.actionSetName, name);
  strcpy(set_info.localizedActionSetName, name);

  XrResult result = xrCreateActionSet (self->instance,
                                       &set_info,
                                       &self->handle);

  if (result != XR_SUCCESS)
    {
      g_printerr ("Failed to create action set: ");
      _printerr_xr_result (self->instance, result);
      g_clear_object (&self);
    }

  return self;
}

static void
openxr_action_set_finalize (GObject *gobject)
{
  OpenXRActionSet *self = (OPENXR_ACTION_SET (gobject));
  g_free (self->url);

  G_OBJECT_CLASS (openxr_action_set_parent_class)->finalize (gobject);
}

static gboolean
_update (GxrActionSet **sets, uint32_t count)
{
  if (count == 0)
    return FALSE;

  XrActiveActionSet *active_action_sets = g_malloc (sizeof (XrActiveActionSet) * count);
  for (uint32_t i = 0; i < count; i++)
    {
      OpenXRActionSet *self = OPENXR_ACTION_SET (sets[i]);
      active_action_sets[i].actionSet = self->handle;
      active_action_sets[i].subactionPath = XR_NULL_PATH;
    }

  XrActionsSyncInfo syncInfo = {
    .type = XR_TYPE_ACTIONS_SYNC_INFO,
    .countActiveActionSets = count,
    .activeActionSets = active_action_sets
  };

  /* All actionsets must be attached to the same session */
  OpenXRActionSet *self = OPENXR_ACTION_SET (sets[0]);

  XrResult result = xrSyncActions (self->session, &syncInfo);

  g_free (active_action_sets);

  if (result != XR_SUCCESS)
    {
      g_printerr ("ERROR: SyncActions: ");
      _printerr_xr_result (self->instance, result);
      return FALSE;
    }

  return TRUE;
}

static OpenXRActionBinding *
_find_binding (OpenXRAction *action,
               OpenXRBinding *binding,
               int actions_per_binding)
{
  for (int i = 0; i < actions_per_binding; i++)
    {
      char *url = openxr_action_get_url (action);
      OpenXRActionBinding *ab = &binding->action_bindings[i];

      if (g_strcmp0 (ab->name, url) == 0)
        {
          if (!ab->bound)
            return NULL;

          return ab;
        }
    }

  char *url = openxr_action_get_url (action);
  g_printerr ("Failed to find action %s in binding: %s\n",
              url, binding->interaction_profile);
  return NULL;
}

static gboolean
_suggest_for_interaction_profile (XrInstance instance,
                                  GSList *actions,
                                  OpenXRBinding *binding,
                                  int actions_per_binding)
{
  XrActionSuggestedBinding *suggested_bindings =
    g_malloc (sizeof (XrActionSuggestedBinding) *
              (unsigned long) ((int)sizeof(actions) * actions_per_binding));

  uint32_t num_suggestion = 0;
  for (GSList *l = actions; l != NULL; l = l->next)
    {
      OpenXRAction *action = OPENXR_ACTION (l->data);
      XrAction handle = openxr_action_get_handle (action);
      char *url = openxr_action_get_url (action);

      OpenXRActionBinding *ab =
        _find_binding (action, binding, actions_per_binding);

      if (!ab)
        {
          // g_debug ("Skipping unbound action %s\n", url);
          continue;
        }

      g_debug ("Action %s %d components: %s %s\n",
               url, ab->num_components,
               ab->component[0], ab->component[1]);

      for (int i = 0; i < ab->num_components; i++)
        {
          XrPath component_path;
          xrStringToPath(instance, ab->component[i], &component_path);

          suggested_bindings[num_suggestion].action = handle;
          suggested_bindings[num_suggestion].binding = component_path;

          num_suggestion++;
        }
    }

  g_debug ("Suggesting %d component bindings\n", num_suggestion);
  XrPath profile_path;
  xrStringToPath(instance, binding->interaction_profile, &profile_path);

  const XrInteractionProfileSuggestedBinding suggestion_info = {
    .type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING,
    .next = NULL,
    .interactionProfile = profile_path,
    .countSuggestedBindings = num_suggestion,
    .suggestedBindings = suggested_bindings
  };

  XrResult result =
    xrSuggestInteractionProfileBindings (instance, &suggestion_info);
  g_free (suggested_bindings);
  if (result != XR_SUCCESS)
    {
      char buffer[XR_MAX_RESULT_STRING_SIZE];
      xrResultToString(instance, result, buffer);
      g_printerr ("ERROR: Suggesting actions for profile %s: %s\n",
                  binding->interaction_profile, buffer);
      return false;
    }

  return true;
}

static gboolean
_attach_bindings (GxrActionSet **sets, uint32_t count)
{
  XrActionSet *handles = g_malloc (sizeof (XrActionSet) * count);

  /* for each input profile we have to suggest bindings for all actions from
     all action sets at once, so collect them first. */
  GSList *all_actions = NULL;

  for (uint32_t i = 0; i < count; i++)
    {
      OpenXRActionSet *self = OPENXR_ACTION_SET (sets[i]);

      g_debug ("Collecting actions from action set %s: \n", self->url);

      handles[i] = self->handle;

      GSList *actions = gxr_action_set_get_actions (GXR_ACTION_SET (self));
      for (GSList *l = actions; l != NULL; l = l->next)
        {
          OpenXRAction *action = OPENXR_ACTION (l->data);
          all_actions = g_slist_append (all_actions, action);
        }
    }


  char **profiles;
  int num_profiles;
  openxr_action_bindings_get_interaction_profiles (&profiles, &num_profiles);

  OpenXRBinding *bindings;
  int num_bindings;
  int actions_per_binding;
  openxr_action_binding_get (&bindings, &num_bindings, &actions_per_binding);

  for (int i = 0; i < num_profiles; i++)
    {
      XrInstance instance = OPENXR_ACTION_SET (sets[0])->instance;

      OpenXRBinding *binding = &bindings[i];
      g_debug ("Suggesting for profile %s\n", binding->interaction_profile);
      _suggest_for_interaction_profile (instance, all_actions,
                                        binding, actions_per_binding);
    }

  g_slist_free (all_actions);

  XrSessionActionSetsAttachInfo attachInfo = {
    .type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO,
    .next = NULL,
    .countActionSets = count,
    .actionSets = handles
  };

  OpenXRActionSet *self = OPENXR_ACTION_SET (sets[0]);

  XrResult result = xrAttachSessionActionSets(self->session, &attachInfo);
  g_free (handles);
  if (result != XR_SUCCESS)
    {
      char buffer[XR_MAX_RESULT_STRING_SIZE];
      xrResultToString(self->instance, result, buffer);
      g_printerr ("ERROR: attaching action set: %s\n", buffer);
      return false;
    }

  g_debug ("Attached %d action sets\n", count);
  return true;
}

static GxrAction*
_create_action (GxrActionSet *self,
                GxrActionType type, char *url)
{
  return (GxrAction*) openxr_action_new_from_type_url (self, type, url);
}

static void
openxr_action_set_class_init (OpenXRActionSetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openxr_action_set_finalize;

  GxrActionSetClass *gxr_action_set_class = GXR_ACTION_SET_CLASS (klass);
  gxr_action_set_class->update = _update;
  gxr_action_set_class->create_action = _create_action;
  gxr_action_set_class->attach_bindings = _attach_bindings;
}
