/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>

#include "openvr-io.h"

#define CACHE_DIR "openvr-glib"
#define RES_BASE_PATH "/res/bindings"

gboolean
_cache_bindings (GString *actions_path)
{
  GString* cache_path = openvr_io_get_cache_path (CACHE_DIR);

  if (!openvr_io_create_directory_if_needed (cache_path->str))
    return FALSE;

  if (!openvr_io_write_resource_to_file (RES_BASE_PATH, cache_path->str,
                                         "actions.json", actions_path))
    return FALSE;

  GString *bindings_path = g_string_new ("");
  if (!openvr_io_write_resource_to_file (RES_BASE_PATH, cache_path->str,
                                         "bindings_vive_controller.json",
                                         bindings_path))
    return FALSE;

  g_string_free (bindings_path, TRUE);
  g_string_free (cache_path, TRUE);

  return TRUE;
}

int
main ()
{
  GString *action_manifest_path = g_string_new ("");
  g_assert (_cache_bindings (action_manifest_path));

  g_print ("Created manifest file: %s\n", action_manifest_path->str);

  g_string_free (action_manifest_path, TRUE);

  return 0;
}
