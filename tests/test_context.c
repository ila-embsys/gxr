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
_test_init_context ()
{
  GxrContext *context = gxr_context_new ("Test Context", 1);
  g_assert_nonnull (context);
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
_test_quit_event ()
{
  GxrContext *context = gxr_context_new ("Test Context", 1);
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
  _test_init_context ();
  _test_quit_event ();
  gxr_backend_shutdown ();
  return 0;
}
