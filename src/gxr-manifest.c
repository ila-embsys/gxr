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
 *    (OpenVRBindingType)type,
 *    (OpenVRBindingMode)mode
 *    (GList (OpenVRBindingInputPath))input path,
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

typedef struct
{
  GxrManifestBindingComponent component;
  gchar *input_path;
} GxrBindingInputPath;

typedef struct
{
  GxrManifestBindingType binding_type;
  GList *input_paths;
  GxrManifestBindingMode mode;
} GxrBinding;

struct _GxrManifest
{
  GObject parent;

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
  GxrBindingInputPath *path = data;
  g_free (path->input_path);
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
}

static GxrManifestBindingType
_get_binding_type (const gchar *type_string)
{
  if (g_str_equal (type_string, "boolean"))
    return BINDING_TYPE_BOOLEAN;
  if (g_str_equal (type_string, "vector2"))
    return BINDING_TYPE_BOOLEAN;
  if (g_str_equal (type_string, "pose"))
    return BINDING_TYPE_POSE;
  if (g_str_equal (type_string, "vibration"))
    return BINDING_TYPE_HAPTIC;

  g_print ("Binding type %s is not known\n", type_string);
  return BINDING_TYPE_UNKNOWN;
}

static GxrManifestBindingMode
_get_binding_mode (const gchar *mode_string)
{
  if (g_str_equal (mode_string, "button"))
    return BINDING_MODE_BUTTON;
  if (g_str_equal (mode_string, "trackpad"))
    return BINDING_MODE_TRACKPAD;
  if (g_str_equal (mode_string, "joystick"))
    return BINDING_MODE_JOYSTICK;

  g_print ("Binding mode %s is not known\n", mode_string);
  return BINDING_MODE_UNKNOWN;
}

static GxrManifestBindingComponent
_get_binding_component (const gchar *component_string)
{
  if (g_str_equal (component_string, "click"))
    return BINDING_COMPONENT_CLICK;
  if (g_str_equal (component_string, "position"))
    return BINDING_COMPONENT_POSITION;

  g_print ("Binding component %s is not known\n", component_string);
  return BINDING_COMPONENT_UNKNOWN;
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
      binding->mode = BINDING_MODE_UNKNOWN;
      binding->binding_type = _get_binding_type (binding_type);

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

  JsonObject *jobinding = json_object_get_object_member (joroot, "bindings");

  GList *binding_list = json_object_get_members (jobinding);
  for (GList *l = binding_list; l != NULL; l = l->next)
    {
      const gchar *actionset = l->data;
      g_debug ("Parsing Action Set %s\n", actionset);

      JsonObject *joactionset = json_object_get_object_member (jobinding, actionset);

      JsonArray *jasources = json_object_get_array_member (joactionset, "sources");
      int len = json_array_get_length (jasources);
      for (int i = 0; i < len; i++)
        {
          JsonObject *josource = json_array_get_object_element (jasources, i);

          const gchar *path = json_object_get_string_member (josource, "path");
          const gchar *mode = json_object_get_string_member (josource, "mode");

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
              GxrBindingInputPath *input_path = g_malloc (sizeof (GxrBindingInputPath));
              input_path->component = _get_binding_component (component);
              input_path->input_path = g_strdup (path);
              binding->input_paths = g_list_append (binding->input_paths, input_path);
            }
          g_list_free (input_list);
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

static void
gxr_manifest_finalize (GObject *gobject)
{
  GxrManifest *self = GXR_MANIFEST (gobject);
  g_hash_table_unref (self->actions);
  (void) self;
}
