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

#include "openvr-context.h"
#include "openvr-overlay.h"
#include "openvr-overlay-uploader.h"
#include "openvr-math.h"

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
test_color (OpenVROverlay *overlay)
{
  graphene_vec3_t *color = graphene_vec3_alloc ();
  g_assert (openvr_overlay_get_color (overlay, color));

  GString *str = openvr_math_vec3_to_string (color);
  g_print ("Initial color is %s\n", str->str);

  graphene_vec3_init (color, 1.0f, 0.5f, 0.5f);

  g_assert (openvr_overlay_set_color (overlay, color));

  graphene_vec3_t *color_ret = graphene_vec3_alloc ();
  g_assert (openvr_overlay_get_color (overlay, color_ret));

  str = openvr_math_vec3_to_string (color_ret);
  g_print ("We have set color to %s\n", str->str);

  g_assert (graphene_vec3_equal (color, color_ret));

  g_string_free (str, TRUE);
  graphene_vec3_free (color);
  graphene_vec3_free (color_ret);
}

void
test_overlay_pixbuf ()
{
  GError *error = NULL;
  GdkPixbuf * pixbuf = load_gdk_pixbuf ();
  g_assert (error == NULL);
  g_assert_nonnull (pixbuf);

  g_assert (openvr_context_is_installed ());
  OpenVRContext *context = openvr_context_get_instance ();
  g_assert_nonnull (context);
  g_assert (openvr_context_init_overlay (context));
  g_assert (openvr_context_is_valid (context));

  OpenVROverlayUploader *uploader = openvr_overlay_uploader_new ();
  g_assert_nonnull (uploader);

  g_assert (openvr_overlay_uploader_init_vulkan (uploader, true));

  GulkanClient *client = GULKAN_CLIENT (uploader);
  GulkanDevice *device = gulkan_client_get_device (client);

  GulkanTexture *texture =
    gulkan_texture_new_from_pixbuf (device, pixbuf,
                                    VK_FORMAT_R8G8B8A8_UNORM);
  g_assert_nonnull (texture);

  gulkan_client_upload_pixbuf (client, texture, pixbuf);

  OpenVROverlay *overlay = openvr_overlay_new ();
  g_assert_nonnull (overlay);

  openvr_overlay_create (overlay, "test.pixbuf", "GDK pixbuf");

  g_assert (openvr_overlay_is_valid (overlay));

  openvr_overlay_set_mouse_scale (overlay,
                                  (float) gdk_pixbuf_get_width (pixbuf),
                                  (float) gdk_pixbuf_get_height (pixbuf));

  test_color (overlay);

  openvr_overlay_uploader_submit_frame (uploader, overlay, texture);

  g_object_unref (pixbuf);
}

int
main ()
{
  test_overlay_pixbuf ();

  return 0;
}
