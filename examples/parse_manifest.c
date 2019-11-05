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
    g_resources_open_stream ("/res/bindings/actions.json",
                             G_RESOURCE_LOOKUP_FLAGS_NONE,
                             &error);
    if (error)
      {
        g_print ("Unable to load action resource: %s\n", error->message);
        g_error_free (error);
        return 1;
      }

    GInputStream *bindings_stream =
      g_resources_open_stream ("/res/bindings/bindings_knuckles_controller.json",
                               G_RESOURCE_LOOKUP_FLAGS_NONE,
                               &error);
      if (error)
      {
        g_print ("Unable to load binding resource: %s\n", error->message);
        g_error_free (error);
        return 1;
      }


  GxrManifest *manifest = gxr_manifest_new ();
  gxr_manifest_load (manifest, actions_stream, bindings_stream);

  g_object_unref (actions_stream);

  g_object_unref (manifest);
}
