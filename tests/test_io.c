/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>

#include "gxr.h"

#define CACHE_DIR "gxr"
#define RES_BASE_PATH "/res/bindings/openxr"

static gboolean
_cache_bindings (GString *actions_path)
{
  GString* cache_path = gxr_io_get_cache_path (CACHE_DIR);

  if (g_mkdir_with_parents (cache_path->str, 0700) == -1)
    {
      g_printerr ("Unable to create directory %s\n", cache_path->str);
      return FALSE;
    }

  if (!gxr_io_write_resource_to_file (RES_BASE_PATH, cache_path->str,
                                      "actions.json", actions_path))
    return FALSE;

  GString *bindings_path = g_string_new ("");
  if (!gxr_io_write_resource_to_file (RES_BASE_PATH, cache_path->str,
                                      "bindings_valve_index_controller.json",
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
