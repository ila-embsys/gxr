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
  GxrContext *context = gxr_context_new ();
  g_assert_nonnull (context);

  GulkanClient *gc = gulkan_client_new ();
  g_assert_nonnull (gc);

  g_assert (gxr_context_inititalize (context, gc, GXR_APP_SCENE));
  g_assert (gxr_context_is_valid (context));


  if (gxr_context_get_api (context) == GXR_API_OPENVR)
  {
    if (!gxr_context_load_action_manifest (
      context,
      "xrdesktop.openvr",
      "/res/bindings/openvr",
      "actions.json",
      "bindings_vive_controller.json",
      "bindings_knuckles_controller.json",
      NULL))
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
        "actions.json",
        "bindings_khronos_simple_controller.json",
        "bindings_valve_index_controller.json",
        NULL))
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
