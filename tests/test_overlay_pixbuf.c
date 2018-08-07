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
#include "openvr-vulkan-uploader.h"

GdkPixbuf *
load_gdk_pixbuf ()
{
  GError *error = NULL;
  GdkPixbuf * pixbuf_rgb =
    gdk_pixbuf_new_from_resource ("/res/cat.jpg", &error);

  if (error != NULL)
    {
      g_printerr ("Unable to read file: %s\n", error->message);
      g_error_free (error);
      return NULL;
    }
  else
    {
      GdkPixbuf *pixbuf = gdk_pixbuf_add_alpha (pixbuf_rgb, false, 0, 0, 0);
      g_object_unref (pixbuf_rgb);
      return pixbuf;
    }
  }

void
test_overlay_opengl_pixbuf ()
{
  GError *error = NULL;
  GdkPixbuf * pixbuf = load_gdk_pixbuf ();
  g_assert (error == NULL);
  g_assert_nonnull (pixbuf);

  OpenVRSystem * system = openvr_system_new ();
  g_assert_nonnull (system);
  g_assert (openvr_system_init_overlay (system));

  g_assert (openvr_system_is_available (system));
  g_assert (openvr_system_is_installed ());

  OpenVRCompositor *compositor = openvr_compositor_new ();
  g_assert_nonnull (compositor);

  OpenVRVulkanUploader *uploader = openvr_vulkan_uploader_new ();
  g_assert_nonnull (uploader);

  g_assert (openvr_vulkan_uploader_init_vulkan (uploader, true,
                                                system, compositor));

  OpenVRVulkanTexture *texture = openvr_vulkan_texture_new ();
  g_assert_nonnull (texture);

  openvr_vulkan_client_load_pixbuf (OPENVR_VULKAN_CLIENT (uploader),
                                    texture, pixbuf);

  OpenVROverlay *overlay = openvr_overlay_new ();
  g_assert_nonnull (overlay);
  g_assert (openvr_overlay_is_available (overlay));

  openvr_overlay_create (overlay, "test.pixbuf", "GDK pixbuf",
                         ETrackingUniverseOrigin_TrackingUniverseStanding);

  g_assert (openvr_overlay_is_valid (overlay));

  openvr_overlay_set_mouse_scale (overlay,
                                  (float) gdk_pixbuf_get_width (pixbuf),
                                  (float) gdk_pixbuf_get_height (pixbuf));

  openvr_vulkan_uploader_submit_frame (uploader, overlay, texture);

  g_object_unref (pixbuf);
}

int
main (int argc, char *argv[])
{
  test_overlay_opengl_pixbuf ();

  return 0;
}
