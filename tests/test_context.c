/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr.h"
#include <glib.h>
#include <gulkan.h>

static void
_test_init_context ()
{
  GxrContext *context = gxr_context_new ("Test Context", 1);
  g_assert_nonnull (context);
  g_object_unref (context);
}

static void
_test_quit_event ()
{
  GxrContext *context = gxr_context_new ("Test Context", 1);
  g_assert_nonnull (context);

  g_print ("Requesting exit\n");
  gxr_context_request_quit (context);

  g_object_unref (context);

  g_print ("Exit completed\n");
}

int
main ()
{
  _test_init_context ();
  _test_quit_event ();
  return 0;
}
