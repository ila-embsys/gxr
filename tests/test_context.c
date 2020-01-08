/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <gulkan.h>
#include "gxr.h"

#define ENUM_TO_STR(r) case r: return #r

static const gchar*
gxr_api_string (GxrApi v)
{
  switch (v)
    {
      ENUM_TO_STR(GXR_API_OPENXR);
      ENUM_TO_STR(GXR_API_OPENVR);
      default:
        return "UNKNOWN API";
    }
}

static void
_init_context (GxrAppType type)
{
  GxrContext *context = gxr_context_get_instance ();
  g_assert_nonnull (context);

  GxrApi api = gxr_context_get_api (context);
  g_print ("Using API: %s\n", gxr_api_string (api));

  GulkanClient *gc = gulkan_client_new ();

  g_assert (gxr_context_init_runtime (context, type));
  g_assert (gxr_context_init_gulkan (context, gc));
  g_assert (gxr_context_init_session (context, gc));
  g_assert (gxr_context_is_valid (context));

  g_object_unref (context);
}

int
main ()
{
  _init_context (GXR_APP_SCENE);
  _init_context (GXR_APP_OVERLAY);
  return 0;
}
