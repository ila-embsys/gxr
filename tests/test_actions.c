/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <gulkan.h>
#include "gxr.h"

static void
_test_load_action_manifest ()
{
  GxrContext *context = gxr_context_new (GXR_APP_SCENE, "Test Actions", 1);
  g_assert_nonnull (context);

  GulkanClient *gc = gulkan_client_new ();
  g_assert_nonnull (gc);

  if (gxr_context_get_api (context) == GXR_API_OPENVR)
  {
    if (!gxr_context_load_action_manifest (
      context,
      "xrdesktop.openvr",
      "/res/bindings/openvr",
      "actions.json"))
    {
      g_print ("Failed to load action bindings!\n");
      return;
    }
  }
  else
  {
    {
      if (!gxr_context_load_action_manifest (
        context,
        "xrdesktop.openxr",
        "/res/bindings/openxr",
        "actions.json"))
      {
        g_print ("Failed to load action bindings!\n");
        return;
      }
    }
  }

  g_object_unref (context);
}

int
main ()
{
  _test_load_action_manifest ();
  gxr_backend_shutdown ();
  return 0;
}
