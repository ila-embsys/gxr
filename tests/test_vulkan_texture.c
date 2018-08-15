/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-client.h"
#include "openvr-vulkan-texture.h"

GdkPixbuf *
load_gdk_pixbuf ()
{
  GError *error = NULL;
  GdkPixbuf * pixbuf_rgb =
    gdk_pixbuf_new_from_resource ("/res/cat.jpg", &error);

  g_assert_null (error);

  GdkPixbuf *pixbuf = gdk_pixbuf_add_alpha (pixbuf_rgb, false, 0, 0, 0);
  g_object_unref (pixbuf_rgb);
  return pixbuf;
}

void
_test_pixbuf ()
{
  GdkPixbuf * pixbuf = load_gdk_pixbuf ();
  g_assert_nonnull (pixbuf);
  g_object_unref (pixbuf);
}

void
_test_pixbuf_texture ()
{
  GdkPixbuf * pixbuf = load_gdk_pixbuf ();
  g_assert_nonnull (pixbuf);

  OpenVRVulkanClient *client = openvr_vulkan_client_new ();
  g_assert_nonnull (client);

  g_assert (openvr_vulkan_client_init_vulkan (client, NULL, NULL, true));

  OpenVRVulkanTexture *texture = openvr_vulkan_texture_new ();
  g_assert_nonnull (texture);

  openvr_vulkan_client_load_pixbuf (client, texture, pixbuf);

  g_object_unref (texture);
  g_object_unref (client);

  g_object_unref (pixbuf);
}

int
main (int argc, char *argv[])
{
  _test_pixbuf ();
  _test_pixbuf_texture ();

  return 0;
}


