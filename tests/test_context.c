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

static void
_test_scene_available (void)
{
  GxrContext* gxr_context = gxr_context_new_headless ("Test Context", 1);
  gboolean scene_available =
    !gxr_context_is_another_scene_running (gxr_context);
  g_print ("Scene available: %d\n", scene_available);
  g_object_unref (gxr_context);
}

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
_test_init_context (GxrAppType type)
{
  GxrContext *context = gxr_context_new (type, "Test Context", 1);
  g_assert_nonnull (context);

  GxrApi api = gxr_context_get_api (context);
  g_print ("Using API: %s\n", gxr_api_string (api));

  g_object_unref (context);
}

static void
_system_quit_cb (GxrContext   *context,
                 GxrQuitEvent *event,
                 gboolean     *quit_completed)
{
  g_print ("Acknowledging VR quit event %d\n", event->reason);
  gxr_context_acknowledge_quit (context);

  *quit_completed = TRUE;

  g_free (event);
}

static void
_test_quit_event (GxrAppType type)
{
  GxrContext *context = gxr_context_new (type, "Test Context", 1);
  g_assert_nonnull (context);

  gboolean quit_completed = FALSE;
  g_signal_connect (context, "quit-event",
                    (GCallback) _system_quit_cb, &quit_completed);

  g_print ("Requesting exit\n");
  gxr_context_request_quit (context);

  while (!quit_completed)
    {
      gxr_context_poll_event (context);
      g_usleep (100000);
    }

  g_object_unref (context);

  g_print ("Exit completed\n");
}

int
main ()
{
  _test_scene_available ();
  _test_init_context (GXR_APP_SCENE);
  _test_init_context (GXR_APP_OVERLAY);
  _test_quit_event (GXR_APP_SCENE);
  gxr_backend_shutdown ();
  return 0;
}
