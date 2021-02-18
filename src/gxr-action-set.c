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
#include "openxr/openxr-context.h"

struct _GxrActionSet
{
  GObject parent;

  GSList *actions;

  GxrContext *context;
  XrInstance instance;
  XrSession session;

  char *url;

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
}

GxrActionSet *
gxr_action_set_new (GxrContext *context)
{
  GxrActionSet *self =
    (GxrActionSet*) g_object_new (GXR_TYPE_ACTION_SET, 0);

  self->instance = openxr_context_get_openxr_instance (OPENXR_CONTEXT(context));
  self->session = openxr_context_get_openxr_session (OPENXR_CONTEXT(context));
  self->context = context;

  return self;
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
  xrResultToString(instance, result, buffer);
  g_printerr ("%s\n", buffer);
}

GxrActionSet *
gxr_action_set_new_from_url (GxrContext *context, gchar *url)
{
  GxrActionSet *self = gxr_action_set_new (context);
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
gxr_action_set_finalize (GObject *gobject)
{
  GxrActionSet *self = GXR_ACTION_SET (gobject);
  g_slist_free (self->actions);
  g_free (self->url);
  G_OBJECT_CLASS (gxr_action_set_parent_class)->finalize (gobject);
}

static gboolean
_update (GxrActionSet **sets, uint32_t count)
{
  if (count == 0)
    return TRUE;

  /* All actionsets must be attached to the same session */
  XrInstance instance = GXR_ACTION_SET (sets[0])->instance;
  XrSession session = GXR_ACTION_SET (sets[0])->session;

  XrSessionState state =
    openxr_context_get_session_state (OPENXR_CONTEXT (GXR_ACTION_SET (sets[0])->context));
  /* just pretend no input happens when we're not focused */
  if (state != XR_SESSION_STATE_FOCUSED)
    return TRUE;

  XrActiveActionSet *active_action_sets = g_malloc (sizeof (XrActiveActionSet) * count);
  for (uint32_t i = 0; i < count; i++)
    {
      GxrActionSet *self = GXR_ACTION_SET (sets[i]);
      active_action_sets[i].actionSet = self->handle;
      active_action_sets[i].subactionPath = XR_NULL_PATH;
    }

  XrActionsSyncInfo syncInfo = {
    .type = XR_TYPE_ACTIONS_SYNC_INFO,
    .countActiveActionSets = count,
    .activeActionSets = active_action_sets
  };


  XrResult result = xrSyncActions (session, &syncInfo);

  g_free (active_action_sets);

  if (result == XR_SESSION_NOT_FOCUSED)
    {
      /* xrSyncActions can be called before reading the session state change */
      g_debug ("SyncActions while session not focused\n");
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
          GxrAction *action = (GxrAction*) l->data;

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
  GxrAction *action =
    gxr_action_new_from_type_url (context, self,
                                  GXR_ACTION_DIGITAL_FROM_FLOAT, url);

  if (action != NULL)
    self->actions = g_slist_append (self->actions, action);


  GxrAction *haptic_action = NULL;
  if (haptic_url)
    {
      haptic_action = gxr_action_new_from_type_url (context,
                                                    self,
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

static gchar *
_component_to_str (GxrBindingComponent c)
{
  switch (c)
  {
    case GXR_BINDING_COMPONENT_CLICK:
      return "click";
    case GXR_BINDING_COMPONENT_PULL:
      return "value";
    case GXR_BINDING_COMPONENT_TOUCH:
      return "touch";
    case GXR_BINDING_COMPONENT_POSITION:
      /* use .../trackpad instead of .../trackpad/x etc. */
      return NULL;
    default:
      return NULL;
  }
}

static GxrAction *
_find_openxr_action_from_url (GxrActionSet **sets, uint32_t count, gchar *url)
{
  for (uint32_t i = 0; i < count; i++)
    {
      GSList *actions = gxr_action_set_get_actions (GXR_ACTION_SET (sets[i]));
      for (GSList *l = actions; l != NULL; l = l->next)
        {
          GxrAction *action = GXR_ACTION (l->data);
          gchar *action_url = gxr_action_get_url (action);
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

      GxrAction *action = _find_openxr_action_from_url (sets, count, l->data);
      if (!action)
        continue;

      XrAction handle = gxr_action_get_handle (action);
      char *url = gxr_action_get_url (action);

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

          g_debug ("\t%s ", full_path->str);

          XrPath component_path;
          xrStringToPath(instance, full_path->str, &component_path);

          suggested_bindings[num_suggestion].action = handle;
          suggested_bindings[num_suggestion].binding = component_path;

          num_suggestion++;

          g_string_free (full_path, TRUE);
        }
    }
  g_list_free (action_list);

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

static void
_update_controllers (GxrActionSet *self)
{
  GSList *actions = gxr_action_set_get_actions (GXR_ACTION_SET (self));
  for (GSList *l = actions; l != NULL; l = l->next)
  {
    GxrAction *action = GXR_ACTION (l->data);
    gxr_action_update_controllers (action);
  }
}

gboolean
gxr_action_sets_attach_bindings (GxrActionSet **sets,
                                 GxrContext *context,
                                 uint32_t count)
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

      XrInstance instance = sets[0]->instance;

      g_debug ("Suggesting for profile %s\n",
               gxr_manifest_get_interaction_profile (manifest));
      _suggest_for_interaction_profile (sets, count, instance, manifest);
    }

  XrActionSet *handles = g_malloc (sizeof (XrActionSet) * count);
  for (uint32_t i = 0; i < count; i++)
    {
      GxrActionSet *self = sets[i];
      handles[i] = self->handle;
    }


  XrSessionActionSetsAttachInfo attachInfo = {
    .type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO,
    .next = NULL,
    .countActionSets = count,
    .actionSets = handles
  };

  GxrActionSet *self = sets[0];

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

  _update_controllers (self);
  g_debug ("Updating controllers based on actions\n");

  return TRUE;
}

XrActionSet
gxr_action_set_get_handle (GxrActionSet *self)
{
  return self->handle;
}



