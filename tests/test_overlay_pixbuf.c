/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

#include <openvr-glib.h>

#include "openvr-system.h"
#include "openvr-overlay.h"


gboolean
init_offscreen_gl ()
{
  CoglError *error = NULL;
  CoglContext *ctx = cogl_context_new (NULL, &error);
  if (!ctx)
    {
      fprintf (stderr, "Failed to create context: %s\n", error->message);
      return FALSE;
    }

  // TODO: implement CoglOffscreen frameuffer
  CoglOnscreen *onscreen = cogl_onscreen_new (ctx, 800, 600);

  return ctx != NULL && onscreen != NULL;
}

int
test_pixbuf_overlay ()
{
  GError *error = NULL;
  GdkPixbuf * pixbuf = gdk_pixbuf_new_from_resource ("/res/cat.jpg", &error);
  g_assert (error == NULL);
  g_assert_nonnull (pixbuf);

  g_assert (init_offscreen_gl ());

  OpenVRSystem * system = openvr_system_new ();
  g_assert_nonnull (system);
  g_assert (openvr_system_init_overlay (system));

  g_assert (openvr_system_is_available ());
  g_assert (openvr_system_is_compositor_available ());
  g_assert (openvr_system_is_overlay_available ());

  OpenVROverlay *overlay = openvr_overlay_new ();
  g_assert_nonnull (overlay);

  openvr_overlay_create (overlay, "test.pixbuf", "GDK pixbuf");

  g_assert (openvr_overlay_is_valid (overlay));

  openvr_overlay_set_mouse_scale (overlay,
                                  (float) gdk_pixbuf_get_width (pixbuf),
                                  (float) gdk_pixbuf_get_height (pixbuf));

  openvr_overlay_upload_gdk_pixbuf (overlay, pixbuf);

  return 0;
}

int main (int argc, char *argv[]) {
  return test_pixbuf_overlay ();
}
