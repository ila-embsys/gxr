/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-manifest.h"

#include <json-glib/json-glib.h>

/*
 * actions are saved in a hashmap
 * (gchar)action name ->
 *  (struct){
 *    (GxrBindingType)type,
 *    (GxrBindingMode)mode
 *    (GList (GxrBindingInputPath))input_path,
 * }
 *
 * example:
 * "/actions/wm/in/grab_window" ->
 *  {
 *    type: BINDING_TYPE_BOOLEAN,
 *    mode: "button"
 *    [
 *     {
 *        component: "click",
 *        input path: [/user/hand/left/input/trigger, /user/hand/right/input/trigger],
 *      }
 *    ]
 * }
 */

struct _GxrManifest
{
  GObject parent;

  gchar *interaction_profile;
  int num_inputs;
  GHashTable *actions;
};

G_DEFINE_TYPE (GxrManifest, gxr_manifest, G_TYPE_OBJECT)

static void
gxr_manifest_finalize (GObject *gobject);

static void
gxr_manifest_class_init (GxrManifestClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gxr_manifest_finalize;
}

static void
_free_input_path (gpointer data)
{
  GxrBindingPath *path = data;
  g_free (path->path);
  g_free (path);
}

static void
_free_binding (gpointer data)
{
  GxrBinding *binding = data;
  g_list_free_full (binding->input_paths, _free_input_path);
  g_free (binding);
}

static void
gxr_manifest_init (GxrManifest *self)
{
  self->actions = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, _free_binding);
  self->interaction_profile = NULL;
  self->num_inputs = 0;
}

static GxrBindingType
_get_binding_type (const gchar *type_string)
{
  if (g_str_equal (type_string, "boolean"))
    return GXR_BINDING_TYPE_BOOLEAN;
  if (g_str_equal (type_string, "vector1"))
    return GXR_BINDING_TYPE_FLOAT;
  if (g_str_equal (type_string, "vector2"))
    return GXR_BINDING_TYPE_VEC2;
  if (g_str_equal (type_string, "pose"))
    return GXR_BINDING_TYPE_POSE;
  if (g_str_equal (type_string, "vibration"))
    return GXR_BINDING_TYPE_HAPTIC;

  g_print ("Binding type %s is not known\n", type_string);
  return GXR_BINDING_TYPE_UNKNOWN;
}

static GxrBindingMode
_get_binding_mode (const gchar *mode_string)
{
  if (mode_string == NULL)
    return GXR_BINDING_MODE_NONE;
  if (g_str_equal (mode_string, "button"))
    return GXR_BINDING_MODE_BUTTON;
  if (g_str_equal (mode_string, "trackpad"))
    return GXR_BINDING_MODE_TRACKPAD;
  if (g_str_equal (mode_string, "joystick"))
    return GXR_BINDING_MODE_JOYSTICK;

  g_print ("Binding mode %s is not known\n", mode_string);
  return GXR_BINDING_MODE_UNKNOWN;
}

static GxrBindingComponent
_get_binding_component (const gchar *component_string)
{
  if (g_str_equal (component_string, "click"))
    return GXR_BINDING_COMPONENT_CLICK;
  if (g_str_equal (component_string, "pull"))
    return GXR_BINDING_COMPONENT_PULL;
  if (g_str_equal (component_string, "position"))
    return GXR_BINDING_COMPONENT_POSITION;
  if (g_str_equal (component_string, "touch"))
    return GXR_BINDING_COMPONENT_TOUCH;

  g_print ("Binding component %s is not known\n", component_string);
  return GXR_BINDING_COMPONENT_UNKNOWN;
}

static gboolean
_parse_actions (GxrManifest *self, GInputStream *stream)
{
  GError *error = NULL;

  JsonParser *parser = json_parser_new ();
  json_parser_load_from_stream (parser, stream, NULL, &error);
  if (error)
    {
      g_print ("Unable to parse actions: %s\n", error->message);
      g_error_free (error);
      g_object_unref (parser);
      return FALSE;
    }

  JsonNode *jnroot = json_parser_get_root (parser);

  JsonObject *joroot;
  joroot = json_node_get_object (jnroot);

  JsonArray *joactions = json_object_get_array_member (joroot, "actions");

  guint len = json_array_get_length (joactions);
  g_debug ("parsing %d actions\n", len);

  for (guint i = 0; i < len; i++)
    {
      JsonObject *joaction = json_array_get_object_element (joactions, i);
      const gchar *name = json_object_get_string_member (joaction, "name");
      const gchar *binding_type = json_object_get_string_member (joaction, "type");

      g_debug ("Parsed action %s: %s\n", name, binding_type);

      GxrBinding *binding = g_malloc (sizeof (GxrBinding));
      binding->input_paths = NULL;
      binding->mode = GXR_BINDING_MODE_UNKNOWN;
      binding->type = _get_binding_type (binding_type);

      g_hash_table_insert (self->actions, g_strdup (name), binding);
    }
  g_object_unref (parser);

  return TRUE;
}

static gboolean
_parse_bindings (GxrManifest *self, GInputStream *stream)
{
  GError *error = NULL;

  JsonParser *parser = json_parser_new ();
  json_parser_load_from_stream (parser, stream, NULL, &error);
  if (error)
    {
      g_print ("Unable to parse bindings: %s\n", error->message);
      g_error_free (error);
      g_object_unref (parser);
      return FALSE;
    }

  JsonNode *jnroot = json_parser_get_root (parser);

  JsonObject *joroot;
  joroot = json_node_get_object (jnroot);

  if (json_object_has_member (joroot, "interaction_profile"))
    self->interaction_profile = g_strdup (json_object_get_string_member (joroot, "interaction_profile"));


  JsonObject *jobinding = json_object_get_object_member (joroot, "bindings");

  GList *binding_list = json_object_get_members (jobinding);
  for (GList *l = binding_list; l != NULL; l = l->next)
    {
      const gchar *actionset = l->data;
      g_debug ("Parsing Action Set %s\n", actionset);

      JsonObject *joactionset = json_object_get_object_member (jobinding, actionset);

      JsonArray *jasources = json_object_get_array_member (joactionset, "sources");
      guint sources_len = json_array_get_length (jasources);
      for (guint i = 0; i < sources_len; i++)
        {
          JsonObject *josource = json_array_get_object_element (jasources, i);

          const gchar *path = json_object_get_string_member (josource, "path");
          const gchar *mode = NULL;
          if (json_object_has_member (josource, "mode"))
            json_object_get_string_member (josource, "mode");

          g_debug ("Parsed path %s with mode %s\n", path, mode);

          JsonObject *joinputs = json_object_get_object_member (josource, "inputs");

          GList *input_list = json_object_get_members (joinputs);
          for (GList *m = input_list; m != NULL; m = m->next)
            {
              gchar *component = m->data;
              g_debug ("Parsing component %s\n", component);

              JsonObject *joinput = json_object_get_object_member (joinputs, component);

              const gchar *output = json_object_get_string_member (joinput, "output");

              g_debug ("%s: Parsed output %s for component %s\n", path, output, component);

              GxrBinding *binding = g_hash_table_lookup (self->actions, output);
              if (!binding)
                {
                  g_print ("Binding: Failed to find action %s in paresed actions\n", output);
                  continue;
                }

              binding->mode = _get_binding_mode (mode);
              GxrBindingPath *input_path = g_malloc (sizeof (GxrBindingPath));
              input_path->component = _get_binding_component (component);
              input_path->path = g_strdup (path);
              g_debug ("Parsed input path %s\n", input_path->path);
              binding->input_paths = g_list_append (binding->input_paths, input_path);

              self->num_inputs++;
            }
          g_list_free (input_list);
        }


      /* TODO: haptics */

      if (!json_object_has_member (joactionset, "pose"))
        continue;

      JsonArray *japose = json_object_get_array_member (joactionset, "pose");
      guint pose_len = json_array_get_length (japose);
      for (guint i = 0; i < pose_len; i++)
        {
          JsonObject *jopose = json_array_get_object_element (japose, i);

          const gchar *path = json_object_get_string_member (jopose, "path");
          const gchar *output = json_object_get_string_member (jopose, "output");

          g_debug ("%s: Parsed output pose %s \n", path, output);

          GxrBinding *binding = g_hash_table_lookup (self->actions, output);
          if (!binding)
            {
              g_print ("Binding: Failed to find action %s in paresed actions\n", output);
              continue;
            }

          binding->mode = GXR_BINDING_MODE_NONE;
          GxrBindingPath *input_path = g_malloc (sizeof (GxrBindingPath));
          input_path->component = GXR_BINDING_COMPONENT_NONE;
          input_path->path = g_strdup (path);
          g_debug ("Parsed input path %s\n", input_path->path);
          binding->input_paths = g_list_append (binding->input_paths, input_path);

          self->num_inputs++;
        }
    }

  g_list_free (binding_list);

  g_object_unref (parser);

  return TRUE;
}

gboolean
gxr_manifest_load (GxrManifest *self,
                      GInputStream *action_stream,
                      GInputStream *binding_stream)
{
  if (!_parse_actions (self, action_stream))
    return FALSE;

  if (!_parse_bindings (self, binding_stream))
    return FALSE;

  return TRUE;
}

GxrManifest *
gxr_manifest_new (void)
{
  return (GxrManifest *) g_object_new (GXR_TYPE_MANIFEST, 0);
}

gchar *
gxr_manifest_get_interaction_profile (GxrManifest *self)
{
  return self->interaction_profile;
}

/* TODO: might want to make a better API */
GHashTable *
gxr_manifest_get_hash_table (GxrManifest *self)
{
  return self->actions;
}

int
gxr_manifest_get_num_inputs (GxrManifest *self)
{
  return self->num_inputs;
}

static void
gxr_manifest_finalize (GObject *gobject)
{
  GxrManifest *self = GXR_MANIFEST (gobject);
  g_hash_table_unref (self->actions);
  if (self->interaction_profile)
    g_free (self->interaction_profile);
  (void) self;
}
