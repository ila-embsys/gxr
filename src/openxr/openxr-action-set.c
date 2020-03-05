/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Christoph Haag <christop.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <openxr/openxr.h>

#include "openxr-action-set.h"
#include "openxr-action.h"
#include "openxr-context.h"
#include "gxr-manifest.h"

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
}

OpenXRActionSet *
openxr_action_set_new (OpenXRContext *context)
{
  OpenXRActionSet *self =
    (OpenXRActionSet*) g_object_new (OPENXR_TYPE_ACTION_SET, 0);

  self->instance = openxr_context_get_openxr_instance (context);
  self->session = openxr_context_get_openxr_session (context);

  return self;
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
    return FALSE;

  strncpy (name, basename, XR_MAX_ACTION_NAME_SIZE - 1);
  return TRUE;
}

OpenXRActionSet *
openxr_action_set_new_from_url (OpenXRContext *context, gchar *url)
{
  OpenXRActionSet *self = openxr_action_set_new (context);
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

static gchar *
_component_to_str (GxrBindingComponent c)
{
  switch (c)
  {
    case GXR_BINDING_COMPONENT_CLICK:
      return "click";
    case GXR_BINDING_COMPONENT_PULL:
      return "value";
    case GXR_BINDING_COMPONENT_POSITION:
      /* use .../trackpad instead of .../trackpad/x etc. */
      return NULL;
    default:
      return NULL;
  }
}

static OpenXRAction *
_find_openxr_action_from_url (GxrActionSet **sets, uint32_t count, gchar *url)
{
  for (uint32_t i = 0; i < count; i++)
    {
      GSList *actions = gxr_action_set_get_actions (GXR_ACTION_SET (sets[i]));
      for (GSList *l = actions; l != NULL; l = l->next)
        {
          OpenXRAction *action = OPENXR_ACTION (l->data);
          gchar *action_url = openxr_action_get_url (action);
          if (g_strcmp0 (action_url, url) == 0)
            return action;
        }
    }
  g_debug ("Did not find action %s\n", url);
  return NULL;
}

static gboolean
_suggest_for_interaction_profile (GxrActionSet **sets, uint32_t count,
                                  XrInstance instance,
                                  GxrManifest *manifest)
{
  GHashTable *actions = gxr_manifest_get_hash_table (manifest);

  GList *action_list = g_hash_table_get_keys (actions);

  uint32_t num_inputs = (uint32_t) gxr_manifest_get_num_inputs (manifest);
  XrActionSuggestedBinding *suggested_bindings =
    g_malloc (sizeof (XrActionSuggestedBinding) * (unsigned long) num_inputs);

  uint32_t num_suggestion = 0;
  for (GList *l = action_list; l != NULL; l = l->next)
    {

      OpenXRAction *action = _find_openxr_action_from_url (sets, count, l->data);
      if (!action)
        continue;

      XrAction handle = openxr_action_get_handle (action);
      char *url = openxr_action_get_url (action);

      GxrBinding *binding = g_hash_table_lookup (actions, l->data);


      g_debug ("Action %s %d inputs\n",
               url, g_list_length (binding->input_paths));

      for (GList *m = binding->input_paths; m; m = m->next)
        {

          /* shouldn't happen, but sanity test is good */
          if (num_suggestion > num_inputs)
            {
              g_printerr ("Manifest parsed input count %d was too low!\n",
                          num_inputs);
              return FALSE;
            }

          GxrBindingPath *input_path = m->data;

          gchar *component_str = _component_to_str (input_path->component);

          GString *full_path = g_string_new ("");
          if (component_str)
            g_string_printf (full_path, "%s/%s", input_path->path, component_str);
          else
            g_string_append (full_path, input_path->path);

          XrPath component_path;
          xrStringToPath(instance, full_path->str, &component_path);

          suggested_bindings[num_suggestion].action = handle;
          suggested_bindings[num_suggestion].binding = component_path;

          num_suggestion++;

          g_string_free (full_path, TRUE);
        }
    }

  g_debug ("Suggesting %d component bindings\n", num_suggestion);
  XrPath profile_path;
  xrStringToPath (instance,
                  gxr_manifest_get_interaction_profile (manifest),
                  &profile_path);

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
                  gxr_manifest_get_interaction_profile (manifest), buffer);
      return FALSE;
    }

  return TRUE;
}

static gboolean
_attach_bindings (GxrActionSet **sets, GxrContext *context, uint32_t count)
{
  GSList *manifests = openxr_context_get_manifests (OPENXR_CONTEXT (context));
  if (g_slist_length (manifests) == 0)
    {
      g_printerr ("Attaching Action Sets, but no manifests are loaded\n");
      return FALSE;
    }

  for (GSList *l = manifests; l; l = l->next)
    {
      GxrManifest *manifest = GXR_MANIFEST (l->data);

      XrInstance instance = OPENXR_ACTION_SET (sets[0])->instance;

      g_debug ("Suggesting for profile %s\n",
               gxr_manifest_get_interaction_profile (manifest));
      _suggest_for_interaction_profile (sets, count, instance, manifest);
    }

  XrActionSet *handles = g_malloc (sizeof (XrActionSet) * count);
  for (uint32_t i = 0; i < count; i++)
    {
      OpenXRActionSet *self = OPENXR_ACTION_SET (sets[i]);
      handles[i] = self->handle;
    }


  XrSessionActionSetsAttachInfo attachInfo = {
    .type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO,
    .next = NULL,
    .countActionSets = count,
    .actionSets = handles
  };

  OpenXRActionSet *self = OPENXR_ACTION_SET (sets[0]);

  XrResult result = xrAttachSessionActionSets (self->session, &attachInfo);
  g_free (handles);
  if (result != XR_SUCCESS)
    {
      char buffer[XR_MAX_RESULT_STRING_SIZE];
      xrResultToString(self->instance, result, buffer);
      g_printerr ("ERROR: attaching action set: %s\n", buffer);
      return FALSE;
    }

  g_debug ("Attached %d action sets\n", count);
  return TRUE;
}

static GxrAction*
_create_action (GxrActionSet *self, GxrContext *context,
                GxrActionType type, char *url)
{
  return (GxrAction*) openxr_action_new_from_type_url (OPENXR_CONTEXT (context),
                                                       self, type, url);
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
