/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gio/gio.h>

#include "gxr-io.h"
#include "openvr-action.h"

GString*
gxr_io_get_cache_path (const gchar* dir_name)
{
  GString *string = g_string_new ("");
  g_string_printf (string, "%s/%s", g_get_user_cache_dir (), dir_name);
  return string;
}

gboolean
gxr_io_write_resource_to_file (const gchar *res_base_path,
                               gchar *cache_path,
                               const gchar *file_name,
                               GString *file_path)
{
  GError *error = NULL;

  GString *res_path = g_string_new ("");

  g_string_printf (res_path, "%s/%s", res_base_path, file_name);

  GInputStream * res_input_stream =
    g_resources_open_stream (res_path->str,
                             G_RESOURCE_LOOKUP_FLAGS_NONE,
                            &error);

  g_string_free (res_path, TRUE);

  g_string_printf (file_path, "%s/%s", cache_path, file_name);

  GFile *out_file = g_file_new_for_path (file_path->str);

  GFileOutputStream *output_stream;
  output_stream = g_file_replace (out_file,
                                  NULL, FALSE,
                                  G_FILE_CREATE_REPLACE_DESTINATION,
                                  NULL, &error);

  if (error != NULL)
    {
      g_printerr ("Unable to open output stream: %s\n", error->message);
      g_error_free (error);
      return FALSE;
    }

  gssize n_bytes_written =
    g_output_stream_splice (G_OUTPUT_STREAM (output_stream),
                            res_input_stream,
                            (GOutputStreamSpliceFlags)
                              (G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET |
                               G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE),
                            NULL, &error);

  if (error != NULL)
    {
      g_printerr ("Unable to write to output stream: %s\n", error->message);
      g_error_free (error);
      return FALSE;
    }

  if (n_bytes_written == 0)
    {
      g_printerr ("No bytes written\n");
      return FALSE;
    }

  g_object_unref (out_file);

  return TRUE;
}

gboolean
gxr_io_load_cached_action_manifest (const char* cache_name,
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

  if (!openvr_action_load_manifest (actions_path->str))
    return FALSE;

  g_string_free (actions_path, TRUE);

  return TRUE;
}
