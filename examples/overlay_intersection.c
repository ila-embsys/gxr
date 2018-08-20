/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <graphene.h>

#include <openvr-glib.h>

#include "openvr-context.h"
#include "openvr-compositor.h"
#include "openvr-controller.h"
#include "openvr-math.h"
#include "openvr-overlay.h"
#include "openvr-vulkan-uploader.h"

OpenVRVulkanTexture *texture;

ControllerState left;
ControllerState right;

OpenVROverlay *pointer;

gboolean
timeout_callback (gpointer data)
{
  OpenVROverlay *overlay = (OpenVROverlay *)data;

  if (left.initialized)
    trigger_events (&left, overlay);
  else
    init_controller (&left, 0);

  if (right.initialized)
    trigger_events (&right, overlay);
  else
    init_controller (&right, 1);

  openvr_overlay_poll_event (overlay);
  return TRUE;
}

void
print_pixbuf_info (GdkPixbuf *pixbuf)
{
  gint n_channels = gdk_pixbuf_get_n_channels (pixbuf);

  GdkColorspace colorspace = gdk_pixbuf_get_colorspace (pixbuf);

  gint     bits_per_sample = gdk_pixbuf_get_bits_per_sample (pixbuf);
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

GdkPixbuf *
load_gdk_pixbuf ()
{
  GError *   error = NULL;
  GdkPixbuf *pixbuf_unflipped =
      gdk_pixbuf_new_from_resource ("/res/cat.jpg", &error);

  if (error != NULL)
    {
      fprintf (stderr, "Unable to read file: %s\n", error->message);
      g_error_free (error);
      return NULL;
    }
  else
    {
      // GdkPixbuf * pixbuf = gdk_pixbuf_flip (pixbuf_unflipped, FALSE);
      GdkPixbuf *pixbuf =
          gdk_pixbuf_add_alpha (pixbuf_unflipped, false, 0, 0, 0);
      g_object_unref (pixbuf_unflipped);
      // g_object_unref (pixbuf);
      return pixbuf;
    }
}

static void
_move_3d_cb (OpenVROverlay           *overlay,
             struct _motion_event_3d *event,
             gpointer                 data)
{
  (void) overlay;
  (void) data;
  // graphene_quaternion_to_vec4(&event->orientation, &quat);
  /*
  g_print ("controller: %3.4f %3.4f %3.4f; %3.4f %3.4f %3.4f %3.4f\n",
           graphene_vec3_get_x(&event->pos),
           graphene_vec3_get_y(&event->pos),
           graphene_vec3_get_z(&event->pos),
           graphene_vec4_get_x(&quat),
           graphene_vec4_get_y(&quat),
           graphene_vec4_get_z(&quat),
           graphene_vec4_get_w(&quat));
  */
  HmdMatrix34_t translation34;
  openvr_math_graphene_to_matrix34 (&event->transform, &translation34);

  // if we have an intersection point, move the pointer overlay there
  if (event->has_intersection)
    {
      translation34.m[0][3] = event->intersection_point.x;
      translation34.m[1][3] = event->intersection_point.y;
      translation34.m[2][3] = event->intersection_point.z;
      // g_print("%f %f %f\n", translation34.m[0][3], translation34.m[1][3],
      // translation34.m[2][3]);
    }

  OpenVRContext *context = openvr_context_get_instance ();
  int err = context->overlay->SetOverlayTransformAbsolute (
      pointer->overlay_handle, context->origin, &translation34);
  if (err != EVROverlayError_VROverlayError_None)
    {
      g_print("Failed to set overlay transform!\n");
    }

  // TODO: because this is a custom event with a struct that has been allocated
  // specifically for us, we need to free it. Maybe reuse?
  free (event);
}

static void
_scroll_cb (OpenVROverlay *overlay, GdkEventScroll *event, gpointer data)
{
  (void) overlay;
  (void) data;
  g_print ("scroll: %s %f\n",
           event->direction == GDK_SCROLL_UP ? "up" : "down", event->y);
}

static void
_press_cb (OpenVROverlay *overlay, GdkEventButton *event, gpointer data)
{
  (void) overlay;
  (void) data;
  g_print ("press: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
}

static void
_release_cb (OpenVROverlay *overlay, GdkEventButton *event, gpointer data)
{
  (void) overlay;
  (void) data;
  g_print ("release: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
}

static void
_show_cb (OpenVROverlay *overlay, gpointer data)
{
  g_print ("show\n");

  /* skip rendering if the overlay isn't available or visible */
  gboolean is_invisible = !openvr_overlay_is_visible (overlay) &&
                          !openvr_overlay_thumbnail_is_visible (overlay);

  if (!openvr_overlay_is_valid (overlay) || is_invisible)
    return;

  OpenVRVulkanUploader *uploader = (OpenVRVulkanUploader *)data;

  openvr_vulkan_uploader_submit_frame (uploader, overlay, texture);
}

static void
_destroy_cb (OpenVROverlay *overlay, gpointer data)
{
  (void) overlay;
  g_print ("destroy\n");
  GMainLoop *loop = (GMainLoop *)data;
  g_main_loop_quit (loop);
}

void
show_overlay_info (OpenVROverlay *overlay)
{
  struct HmdVector2_t center;
  float               radius;
  EDualAnalogWhich    which = EDualAnalogWhich_k_EDualAnalog_Left;

  OpenVRContext *context = openvr_context_get_instance ();
  struct VR_IVROverlay_FnTable *f = context->overlay;

  EVROverlayError err = f->GetOverlayDualAnalogTransform (
      overlay->overlay_handle, which, &center, &radius);

  if (err != EVROverlayError_VROverlayError_None)
    {
      g_printerr ("Could not GetOverlayDualAnalogTransform: %s\n",
                  f->GetOverlayErrorNameFromEnum (err));
    }

  g_print ("Center [%f, %f] Radius %f\n", center.v[0], center.v[1], radius);

  VROverlayTransformType transform_type;
  err = f->GetOverlayTransformType (overlay->overlay_handle, &transform_type);

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

  err = f->GetOverlayFlag (overlay->overlay_handle,
                           VROverlayFlags_RGSS4X, &anti_alias);

  g_print ("VROverlayFlags_RGSS4X: %d\n", anti_alias);

  TrackingUniverseOrigin tracking_origin;
  HmdMatrix34_t          transform;

  err = f->GetOverlayTransformAbsolute (
    overlay->overlay_handle, &tracking_origin, &transform);

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

  openvr_math_print_matrix34 (transform);

  graphene_point3d_t translation_vec;
  graphene_point3d_init (&translation_vec, -1.3f, 1.5f, -2.7f);

  graphene_matrix_t translation;
  graphene_matrix_init_translate (&translation, &translation_vec);

  graphene_matrix_rotate_y (&translation, -45.4f);

  graphene_matrix_print (&translation);

  HmdMatrix34_t translation34;
  openvr_math_graphene_to_matrix34 (&translation, &translation34);

  openvr_math_print_matrix34 (translation34);

  err = f->SetOverlayTransformAbsolute (
    overlay->overlay_handle, tracking_origin, &translation34);

  graphene_matrix_init_from_matrix (&overlay->transform, &translation);
}

OpenVROverlay *
makePointerOverlay (OpenVRVulkanUploader *uploader)
{
#define X TRUE,
#define _ FALSE,
  gboolean pointerSrc[10 * 10] = {
      X X _ _ _ _ _ _ _ _ _ X X X _ _ _ _ _ _ _ X X X X X _ _ _ _ _ _ X X X X X
          X _ _ _ _ X X X X X X X X _ _ _ X X X X X _ _ _ _ _ X X _ X X X _ _ _
              _ _ _ _ X X X _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ FALSE};
#undef X
#undef _
  int rows = 10;
  int cols = 10;

  guchar data[rows * cols * 4];
  memset (data, 0, rows * cols * 4 * sizeof (guchar));

  for (int row = 0; row < rows; row++)
    {
      for (int col = 0; col < cols; col++)
        {
          int r = (row * cols + col) * 4;
          int g = r + 1;
          int b = r + 2;
          int a = r + 3;
          if (pointerSrc[row * cols + col])
            {
              data[r] = 255;
              data[g] = 0;
              data[b] = 0;
              data[a] = 255;
            }
        }
    }

  GdkPixbuf *pixbuf_pointer = gdk_pixbuf_new_from_data (
      data, GDK_COLORSPACE_RGB, TRUE, 8, cols, rows, 4 * cols, NULL, NULL);
  gsize size = gdk_pixbuf_get_byte_length (pixbuf_pointer);
  g_print ("Pointer: loaded %lu bytes\n", size);
  // GdkPixbuf * pixbuf_pointer = load_gdk_pixbuf_pointer();
  print_pixbuf_info (pixbuf_pointer);

  if (pixbuf_pointer == NULL)
    return NULL;
  OpenVRVulkanTexture *pointertexture = openvr_vulkan_texture_new ();
  openvr_vulkan_client_load_pixbuf (OPENVR_VULKAN_CLIENT (uploader),
                                    pointertexture, pixbuf_pointer);

  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create_width (
      overlay, "vulkan.pointer", "Vulkan Pointer", 0.15);

  if (!openvr_overlay_is_valid (overlay))
    {
      fprintf (stderr, "Overlay unavailable.\n");
      return NULL;
    }

  OpenVRContext *context = openvr_context_get_instance ();
  struct VR_IVROverlay_FnTable *f = context->overlay;
  int err = f->ShowOverlay (overlay->overlay_handle);

  if (err != EVROverlayError_VROverlayError_None)
    {
      g_printerr ("Could not ShowOverlay: %s\n",
                  f->GetOverlayErrorNameFromEnum (err));
    }
  openvr_vulkan_uploader_submit_frame (uploader, overlay, pointertexture);
  return overlay;
}

bool
_init_openvr ()
{
  if (!openvr_context_is_installed ())
    {
      g_printerr ("VR Runtime not installed.\n");
      return false;
    }

  OpenVRContext *context = openvr_context_get_instance ();
  if (!openvr_context_init_overlay (context))
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  if (!openvr_context_is_valid (context))
    {
      g_printerr ("Could not load OpenVR function pointers.\n");
      return false;
    }

  return true;
}

int
test_cat_overlay ()
{
  GMainLoop *loop;

  GdkPixbuf *pixbuf = load_gdk_pixbuf ();
  print_pixbuf_info (pixbuf);

  if (pixbuf == NULL)
    return -1;

  loop = g_main_loop_new (NULL, FALSE);

  /* init openvr */
  if (!_init_openvr ())
    return -1;

  OpenVRVulkanUploader *uploader = openvr_vulkan_uploader_new ();
  if (!openvr_vulkan_uploader_init_vulkan (uploader, true))
    {
      g_printerr ("Unable to initialize Vulkan!\n");
      return false;
    }

  texture = openvr_vulkan_texture_new ();

  openvr_vulkan_client_load_pixbuf (OPENVR_VULKAN_CLIENT (uploader), texture,
                                    pixbuf);

  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create (overlay, "vulkan.cat", "Vulkan Cat");

  if (!openvr_overlay_is_valid (overlay))
    {
      fprintf (stderr, "Overlay unavailable.\n");
      return -1;
    }

  OpenVROverlay *overlay2 = openvr_overlay_new ();
  openvr_overlay_create (overlay2, "vulkan.cat2", "Another Vulkan Cat");

  if (!openvr_overlay_is_valid (overlay2))
    {
      fprintf (stderr, "Overlay 2 unavailable.\n");
      return -1;
    }

  openvr_overlay_set_mouse_scale (overlay,
                                  (float)gdk_pixbuf_get_width (pixbuf),
                                  (float)gdk_pixbuf_get_height (pixbuf));

  if (!openvr_overlay_show (overlay))
    return -1;

  pointer = makePointerOverlay (uploader);

  show_overlay_info (overlay);

  openvr_vulkan_uploader_submit_frame (uploader, overlay, texture);
  openvr_vulkan_uploader_submit_frame (uploader, overlay2, texture);

  /* connect glib callbacks */
  // g_signal_connect (overlay, "motion-notify-event", (GCallback)_move_cb, NULL);
  g_signal_connect (overlay, "motion-notify-event-3d", (GCallback)_move_3d_cb,
                    NULL);
  g_signal_connect (overlay, "button-press-event", (GCallback)_press_cb, loop);
  g_signal_connect (overlay, "scroll-event", (GCallback)_scroll_cb, loop);
  g_signal_connect (overlay, "button-release-event", (GCallback)_release_cb,
                    NULL);
  g_signal_connect (overlay, "show", (GCallback)_show_cb, uploader);
  g_signal_connect (overlay, "destroy", (GCallback)_destroy_cb, loop);

  g_timeout_add (20, timeout_callback, overlay);

  /* start glib main loop */
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_print ("bye\n");

  g_object_unref (overlay);
  g_object_unref (pixbuf);
  g_object_unref (texture);
  g_object_unref (uploader);

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  return 0;
}

int
main ()
{
  left.initialized = FALSE;
  right.initialized = FALSE;
  return test_cat_overlay ();
}
