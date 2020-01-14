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
test_overlay_pixbuf ()
{
  GError *error = NULL;
  GdkPixbuf * pixbuf = load_gdk_pixbuf ();
  g_assert (error == NULL);
  g_assert_nonnull (pixbuf);

  GxrContext *context = gxr_context_new ();
  g_assert_nonnull (context);

  GulkanClient *uploader = gulkan_client_new ();

  g_assert (gxr_context_init_runtime (context, GXR_APP_OVERLAY));
  g_assert (gxr_context_init_gulkan (context, uploader));
  g_assert (gxr_context_init_session (context, uploader));
  g_assert (gxr_context_is_valid (GXR_CONTEXT (context)));


  GulkanTexture *texture =
    gulkan_client_texture_new_from_pixbuf (uploader, pixbuf,
                                           VK_FORMAT_R8G8B8A8_UNORM,
                                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                           false);
  g_assert_nonnull (texture);

  GxrOverlay *overlay = gxr_overlay_new (context);
  g_assert_nonnull (overlay);

  gxr_overlay_create (overlay, "test.pixbuf", "GDK pixbuf");

  g_assert (gxr_overlay_is_valid (overlay));

  gxr_overlay_set_mouse_scale (overlay,
                               gdk_pixbuf_get_width (pixbuf),
                               gdk_pixbuf_get_height (pixbuf));

  gxr_overlay_submit_texture (overlay, uploader, texture);

  g_object_unref (pixbuf);
}

int
main ()
{
  test_overlay_pixbuf ();

  return 0;
}
