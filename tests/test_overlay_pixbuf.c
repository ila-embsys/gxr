/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

#include "gxr.h"

static GdkPixbuf *
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

static void
test_color (OpenVROverlay *overlay)
{
  graphene_vec3_t *color = graphene_vec3_alloc ();
  g_assert (openvr_overlay_get_color (overlay, color));

  graphene_vec3_init (color, 1.0f, 0.5f, 0.5f);

  g_assert (openvr_overlay_set_color (overlay, color));

  graphene_vec3_t *color_ret = graphene_vec3_alloc ();
  g_assert (openvr_overlay_get_color (overlay, color_ret));

  g_assert (graphene_vec3_equal (color, color_ret));

  graphene_vec3_free (color);
  graphene_vec3_free (color_ret);
}

static void
test_overlay_pixbuf ()
{
  GError *error = NULL;
  GdkPixbuf * pixbuf = load_gdk_pixbuf ();
  g_assert (error == NULL);
  g_assert_nonnull (pixbuf);

  g_assert (openvr_context_is_installed ());
  OpenVRContext *context = OPENVR_CONTEXT (gxr_context_get_instance ());
  g_assert_nonnull (context);
  g_assert (openvr_context_initialize (context, GXR_APP_OVERLAY));
  g_assert (openvr_context_is_valid (context));

  GulkanClient *uploader = openvr_compositor_gulkan_client_new ();
  g_assert_nonnull (uploader);

  GulkanClient *client = GULKAN_CLIENT (uploader);

  GulkanTexture *texture =
    gulkan_client_texture_new_from_pixbuf (client, pixbuf,
                                           VK_FORMAT_R8G8B8A8_UNORM,
                                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                           false);
  g_assert_nonnull (texture);

  OpenVROverlay *overlay = openvr_overlay_new ();
  g_assert_nonnull (overlay);

  openvr_overlay_create (overlay, "test.pixbuf", "GDK pixbuf");

  g_assert (openvr_overlay_is_valid (overlay));

  openvr_overlay_set_mouse_scale (overlay,
                                  gdk_pixbuf_get_width (pixbuf),
                                  gdk_pixbuf_get_height (pixbuf));

  test_color (overlay);

  openvr_overlay_submit_texture (overlay, uploader, texture);

  g_object_unref (pixbuf);
}

int
main ()
{
  test_overlay_pixbuf ();

  return 0;
}
