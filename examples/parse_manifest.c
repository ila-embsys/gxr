/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gxr.h>

int
main ()
{
  GError *error = NULL;
  GInputStream *actions_stream =
    g_resources_open_stream ("/res/bindings/openxr/actions.json",
                             G_RESOURCE_LOOKUP_FLAGS_NONE,
                             &error);
    if (error)
      {
        g_print ("Unable to load action resource: %s\n", error->message);
        g_error_free (error);
        return 1;
      }
  GxrManifest *manifest = gxr_manifest_new ();
  gxr_manifest_load_actions (manifest, actions_stream);

  gxr_manifest_load_bindings (manifest, "/res/bindings/openxr");

  g_object_unref (actions_stream);

  g_object_unref (manifest);
}
