/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <graphene.h>

#include "gxr.h"

static GulkanTexture *texture;

static gboolean
timeout_callback (gpointer data)
{
  OpenVROverlay *overlay = (OpenVROverlay*) data;
  openvr_overlay_poll_event (overlay);
  return TRUE;
}

static void
print_pixbuf_info (GdkPixbuf * pixbuf)
{
  gint n_channels = gdk_pixbuf_get_n_channels (pixbuf);

  GdkColorspace colorspace = gdk_pixbuf_get_colorspace (pixbuf);

  gint bits_per_sample = gdk_pixbuf_get_bits_per_sample (pixbuf);
  gboolean has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);

  gint width = gdk_pixbuf_get_width (pixbuf);
  gint height = gdk_pixbuf_get_height (pixbuf);

  gint rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  g_print ("We loaded a pixbuf.\n");
  g_print ("channels %d\n", n_channels);
  g_print ("colorspace %d\n", colorspace);
  g_print ("bits_per_sample %d\n", bits_per_sample);
  g_print ("has_alpha %d\n", has_alpha);
  g_print ("pixel dimensions %dx%d\n", width, height);
  g_print ("rowstride %d\n", rowstride);
}

static GdkPixbuf *
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
    //GdkPixbuf * pixbuf = gdk_pixbuf_flip (pixbuf_unflipped, FALSE);
    GdkPixbuf *pixbuf = gdk_pixbuf_add_alpha (pixbuf_unflipped, false, 0, 0, 0);
    g_object_unref (pixbuf_unflipped);
    //g_object_unref (pixbuf);
    return pixbuf;
  }
}

static void
_press_cb (OpenVROverlay  *overlay,
           GdkEventButton *event,
           gpointer        data)
{
  (void) overlay;
  g_print ("press: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}

static void
_show_cb (OpenVROverlay *overlay,
          gpointer      _client)
{
  g_print ("show\n");

  /* skip rendering if the overlay isn't available or visible */
  gboolean is_invisible = !openvr_overlay_is_visible (overlay) &&
                          !openvr_overlay_thumbnail_is_visible (overlay);

  if (!openvr_overlay_is_valid (overlay) || is_invisible)
    return;

  GulkanClient *client = (GulkanClient*) _client;
  openvr_overlay_submit_texture (overlay, client, texture);
}

static void
_destroy_cb (OpenVROverlay *overlay,
             gpointer       data)
{
  (void) overlay;
  g_print ("destroy\n");
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}

static void
show_overlay_info (OpenVROverlay *overlay)
{
  struct HmdVector2_t center;
  float radius;
  EDualAnalogWhich which = EDualAnalogWhich_k_EDualAnalog_Left;

  OpenVRContext *context = openvr_context_get_instance ();
  struct VR_IVROverlay_FnTable *f = context->overlay;

  VROverlayHandle_t overlay_handle = openvr_overlay_get_handle (overlay);
  EVROverlayError err = f->GetOverlayDualAnalogTransform (
    overlay_handle, which, &center, &radius);

  if (err != EVROverlayError_VROverlayError_None)
    {
      g_printerr ("Could not GetOverlayDualAnalogTransform: %s\n",
                  f->GetOverlayErrorNameFromEnum (err));
    }

  g_print ("Center [%f, %f] Radius %f\n", center.v[0], center.v[1], radius);

  VROverlayTransformType transform_type;
  err = f->GetOverlayTransformType (overlay_handle, &transform_type);

  switch (transform_type)
    {
    case VROverlayTransformType_VROverlayTransform_Absolute:
      g_print ("VROverlayTransform_Absolute\n");
      break;
    case VROverlayTransformType_VROverlayTransform_TrackedDeviceRelative:
      g_print ("VROverlayTransform_TrackedDeviceRelative\n");
      break;
    case VROverlayTransformType_VROverlayTransform_SystemOverlay:
      g_print ("VROverlayTransform_SystemOverlay\n");
      break;
    case VROverlayTransformType_VROverlayTransform_TrackedComponent:
      g_print ("VROverlayTransform_TrackedComponent\n");
      break;
    }

  bool anti_alias = false;

  err = f->GetOverlayFlag (overlay_handle,
                           VROverlayFlags_RGSS4X,
                          &anti_alias);

  g_print ("VROverlayFlags_RGSS4X: %d\n", anti_alias);

  TrackingUniverseOrigin tracking_origin;
  HmdMatrix34_t transform;

  err = f->GetOverlayTransformAbsolute (
    overlay_handle,
    &tracking_origin,
    &transform);

  switch (tracking_origin)
    {
    case ETrackingUniverseOrigin_TrackingUniverseSeated:
      g_print ("ETrackingUniverseOrigin_TrackingUniverseSeated\n");
      break;
    case ETrackingUniverseOrigin_TrackingUniverseStanding:
      g_print ("ETrackingUniverseOrigin_TrackingUniverseStanding\n");
      break;
    case ETrackingUniverseOrigin_TrackingUniverseRawAndUncalibrated:
      g_print ("ETrackingUniverseOrigin_TrackingUniverseRawAndUncalibrated\n");
      break;
    }

  gxr_math_print_matrix34 (transform);

  graphene_point3d_t translation_vec;
  graphene_point3d_init (&translation_vec, 1.1f, 0.5f, 0.1f);

  graphene_matrix_t translation;
  graphene_matrix_init_translate (&translation, &translation_vec);

  graphene_matrix_rotate_y (&translation, -30.4f);

  graphene_matrix_print (&translation);

  HmdMatrix34_t translation34;
  gxr_math_graphene_to_matrix34 (&translation, &translation34);

  gxr_math_print_matrix34 (translation34);

  err = f->SetOverlayTransformAbsolute (
    overlay_handle,
    tracking_origin,
    &translation34);
}

static bool
_init_openvr ()
{
  OpenVRContext *context = openvr_context_get_instance ();
  if (!openvr_context_initialize (context, OPENVR_APP_OVERLAY))
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  return true;
}

static int
test_cat_overlay ()
{
  GMainLoop *loop;

  GdkPixbuf * pixbuf = load_gdk_pixbuf ();
  print_pixbuf_info (pixbuf);

  if (pixbuf == NULL)
    return -1;

  loop = g_main_loop_new (NULL, FALSE);

  /* init openvr */
  if (!_init_openvr ())
    return -1;

  GulkanClient *client = openvr_compositor_gulkan_client_new ();
  if (!client)
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return false;
  }

  texture = gulkan_client_texture_new_from_pixbuf (client, pixbuf,
                                                   VK_FORMAT_R8G8B8A8_UNORM,
                                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                   true);

  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create (overlay, "vulkan.cat", "Vulkan Cat");

  if (!openvr_overlay_is_valid (overlay))
  {
    g_printerr ("Overlay 1 handle invalid.\n");
    return -1;
  }

  OpenVROverlay *overlay2 = openvr_overlay_new ();
  openvr_overlay_create (overlay2, "vulkan.cat2", "Another Vulkan Cat");

  if (!openvr_overlay_is_valid (overlay2))
  {
    g_printerr ("Overlay 2 handle invalid.\n");
    return -1;
  }

  openvr_overlay_set_mouse_scale (overlay,
                                  gdk_pixbuf_get_width (pixbuf),
                                  gdk_pixbuf_get_height (pixbuf));

  if (!openvr_overlay_show (overlay))
    return -1;

  if (!openvr_overlay_show (overlay2))
    return -1;

  show_overlay_info (overlay);

  openvr_overlay_submit_texture (overlay, client, texture);
  openvr_overlay_submit_texture (overlay2, client, texture);

  /* connect glib callbacks */
  g_signal_connect (overlay, "button-press-event", (GCallback) _press_cb, loop);
  g_signal_connect (overlay, "show", (GCallback) _show_cb, client);
  g_signal_connect (overlay, "destroy", (GCallback) _destroy_cb, loop);

  g_timeout_add (20, timeout_callback, overlay);

  /* start glib main loop */
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_print ("bye\n");

  g_object_unref (overlay);
  g_object_unref (pixbuf);
  g_object_unref (texture);
  g_object_unref (client);

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  return 0;
}

int main () {
  return test_cat_overlay ();
}
