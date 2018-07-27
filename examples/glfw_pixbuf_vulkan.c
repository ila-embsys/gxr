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
#include "openvr-compositor.h"
#include "openvr-vulkan-renderer.h"

OpenVRVulkanTexture *texture;

GdkPixbuf *
load_gdk_pixbuf ()
{
  GError *error = NULL;
  GdkPixbuf * pixbuf_unflipped =
    gdk_pixbuf_new_from_resource ("/res/cat.jpg", &error);

  if (error != NULL) {
    fprintf (stderr, "Unable to read file: %s\n", error->message);
    g_error_free (error);
    return NULL;
  } else {
    GdkPixbuf *pixbuf = gdk_pixbuf_add_alpha (pixbuf_unflipped, false, 0, 0, 0);
    g_object_unref (pixbuf_unflipped);
    return pixbuf;
  }
}

/*
static void
_destroy_cb (OpenVROverlay *overlay,
             gpointer       data)
{
  g_print ("destroy\n");
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}
*/

int
test_cat_overlay ()
{
  GMainLoop *loop;

  GdkPixbuf * pixbuf = load_gdk_pixbuf ();

  if (pixbuf == NULL)
    return -1;

  loop = g_main_loop_new (NULL, FALSE);

  OpenVRVulkanRenderer *renderer = openvr_vulkan_renderer_new ();
  if (!openvr_vulkan_renderer_init_vulkan (renderer, true))
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return false;
  }

  texture = openvr_vulkan_texture_new ();

  openvr_vulkan_renderer_load_pixbuf (renderer, texture, pixbuf);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_print ("bye\n");

  g_object_unref (pixbuf);
  g_object_unref (texture);
  g_object_unref (renderer);

  return 0;
}

int main (int argc, char *argv[]) {
  return test_cat_overlay ();
}
