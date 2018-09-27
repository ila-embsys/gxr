/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <time.h>
#include <glib/gprintf.h>

#include <gdk/gdk.h>

#include "openvr-context.h"
#include "openvr-overlay.h"
#include <openvr_capi.h>
#include "openvr_capi_global.h"

#include "openvr-time.h"
#include "openvr-math.h"

G_DEFINE_TYPE (OpenVROverlay, openvr_overlay, G_TYPE_OBJECT)

#define GET_OVERLAY_FUNCTIONS \
  EVROverlayError err; \
  OpenVRContext *context = openvr_context_get_instance (); \
  struct VR_IVROverlay_FnTable *f = context->overlay;

#define OVERLAY_CHECK_ERROR(fun, res) \
{ \
  EVROverlayError r = (res); \
  if (r != EVROverlayError_VROverlayError_None) \
    { \
      g_printerr ("ERROR: " fun ": failed with %s in %s:%d\n", \
                  f->GetOverlayErrorNameFromEnum (r), __FILE__, __LINE__); \
      return FALSE; \
    } \
}

enum {
  MOTION_NOTIFY_EVENT,
  BUTTON_PRESS_EVENT,
  BUTTON_RELEASE_EVENT,
  SHOW,
  DESTROY,
  INTERSECTION_EVENT,
  SCROLL_EVENT,
  LAST_SIGNAL
};

static guint overlay_signals[LAST_SIGNAL] = { 0 };

static void
openvr_overlay_class_init (OpenVROverlayClass *klass)
{
  overlay_signals[MOTION_NOTIFY_EVENT] =
    g_signal_new ("motion-notify-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  overlay_signals[BUTTON_PRESS_EVENT] =
    g_signal_new ("button-press-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  overlay_signals[BUTTON_RELEASE_EVENT] =
    g_signal_new ("button-release-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  overlay_signals[SHOW] =
    g_signal_new ("show",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_FIRST,
                   0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  overlay_signals[DESTROY] =
    g_signal_new ("destroy",
                   G_TYPE_FROM_CLASS (klass),
                     G_SIGNAL_RUN_CLEANUP |
                      G_SIGNAL_NO_RECURSE |
                      G_SIGNAL_NO_HOOKS,
                   0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  overlay_signals[INTERSECTION_EVENT] =
    g_signal_new ("intersection-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  overlay_signals[SCROLL_EVENT] =
    g_signal_new ("scroll-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

}

static void
openvr_overlay_init (OpenVROverlay *self)
{
  self->overlay_handle = 0;
  self->thumbnail_handle = 0;
}

OpenVROverlay *
openvr_overlay_new (void)
{
  return (OpenVROverlay*) g_object_new (OPENVR_TYPE_OVERLAY, 0);
}

gboolean
openvr_overlay_create (OpenVROverlay *self, gchar* key, gchar* name)
{
  GET_OVERLAY_FUNCTIONS

  err = f->CreateOverlay (key, name, &self->overlay_handle);

  OVERLAY_CHECK_ERROR ("CreateOverlay", err);

  return TRUE;
}

gboolean
openvr_overlay_create_width (OpenVROverlay *self,
                             gchar* key,
                             gchar* name,
                             float meters)
{
  if (!openvr_overlay_create (self, key, name))
    return FALSE;

  if (!openvr_overlay_set_width_meters (self, meters))
    return FALSE;

  return TRUE;
}

gboolean
openvr_overlay_create_for_dashboard (OpenVROverlay *self,
                                     gchar* key,
                                     gchar* name)
{
  GET_OVERLAY_FUNCTIONS

  err = f->CreateDashboardOverlay (key, name, &self->overlay_handle,
                                  &self->thumbnail_handle);

  OVERLAY_CHECK_ERROR ("CreateDashboardOverlay", err);

  return TRUE;
}

gboolean
openvr_overlay_is_valid (OpenVROverlay *self)
{
  return self->overlay_handle != k_ulOverlayHandleInvalid;
}

gboolean
openvr_overlay_show (OpenVROverlay *self)
{
  GET_OVERLAY_FUNCTIONS

  err = f->ShowOverlay (self->overlay_handle);

  OVERLAY_CHECK_ERROR ("ShowOverlay", err);
  return TRUE;
}

gboolean
openvr_overlay_hide (OpenVROverlay *self)
{
  GET_OVERLAY_FUNCTIONS

  err = f->HideOverlay (self->overlay_handle);

  OVERLAY_CHECK_ERROR ("HideOverlay", err);
  return TRUE;
}

gboolean
openvr_overlay_set_visibility (OpenVROverlay *self, gboolean visibility)
{
  if (visibility)
    return openvr_overlay_show (self);
  else
    return openvr_overlay_hide (self);
}

gboolean
openvr_overlay_is_visible (OpenVROverlay *self)
{
  OpenVRContext *context = openvr_context_get_instance ();
  return context->overlay->IsOverlayVisible (self->overlay_handle);
}

gboolean
openvr_overlay_thumbnail_is_visible (OpenVROverlay *self)
{
  OpenVRContext *context = openvr_context_get_instance ();
  return context->overlay->IsOverlayVisible (self->thumbnail_handle);
}

gboolean
openvr_overlay_set_sort_order (OpenVROverlay *self, uint32_t sort_order)
{
  GET_OVERLAY_FUNCTIONS

  err = f->SetOverlaySortOrder (self->overlay_handle, sort_order);

  OVERLAY_CHECK_ERROR ("SetOverlaySortOrder", err);
  return TRUE;
}

static guint
_vr_to_gdk_mouse_button (uint32_t btn)
{
  switch(btn)
  {
    case EVRMouseButton_VRMouseButton_Left:
      return 0;
    case EVRMouseButton_VRMouseButton_Right:
      return 1;
    case EVRMouseButton_VRMouseButton_Middle:
      return 2;
    default:
      return 0;
  }
}

gboolean
openvr_overlay_enable_mouse_input (OpenVROverlay *self)
{
  GET_OVERLAY_FUNCTIONS

  err = f->SetOverlayInputMethod (self->overlay_handle,
                                  VROverlayInputMethod_Mouse);

  OVERLAY_CHECK_ERROR ("SetOverlayInputMethod", err);

  return TRUE;
}

void
openvr_overlay_poll_event (OpenVROverlay *self)
{
  struct VREvent_t vr_event;

  OpenVRContext *context = openvr_context_get_instance ();

  while (context->overlay->PollNextOverlayEvent (self->overlay_handle,
                                                &vr_event,
                                                 sizeof (vr_event)))
  {
    switch (vr_event.eventType)
    {
      case EVREventType_VREvent_MouseMove:
      {
        GdkEvent *event = gdk_event_new (GDK_MOTION_NOTIFY);
        event->motion.x = vr_event.data.mouse.x;
        event->motion.y = vr_event.data.mouse.y;
        event->motion.time =
          openvr_time_age_secs_to_monotonic_msecs (vr_event.eventAgeSeconds);
        g_signal_emit (self, overlay_signals[MOTION_NOTIFY_EVENT], 0, event);
      } break;

      case EVREventType_VREvent_MouseButtonDown:
      {
        GdkEvent *event = gdk_event_new (GDK_BUTTON_PRESS);
        event->button.x = vr_event.data.mouse.x;
        event->button.y = vr_event.data.mouse.y;
        event->button.time =
          openvr_time_age_secs_to_monotonic_msecs (vr_event.eventAgeSeconds);
        event->button.button =
          _vr_to_gdk_mouse_button (vr_event.data.mouse.button);
        g_signal_emit (self, overlay_signals[BUTTON_PRESS_EVENT], 0, event);
      } break;

      case EVREventType_VREvent_MouseButtonUp:
      {
        GdkEvent *event = gdk_event_new (GDK_BUTTON_RELEASE);
        event->button.x = vr_event.data.mouse.x;
        event->button.y = vr_event.data.mouse.y;
        event->button.time =
          openvr_time_age_secs_to_monotonic_msecs (vr_event.eventAgeSeconds);
        event->button.button =
          _vr_to_gdk_mouse_button (vr_event.data.mouse.button);
        g_signal_emit (self, overlay_signals[BUTTON_RELEASE_EVENT], 0, event);
      } break;

      case EVREventType_VREvent_OverlayShown:
        g_signal_emit (self, overlay_signals[SHOW], 0);
        break;
      case EVREventType_VREvent_Quit:
        g_signal_emit (self, overlay_signals[DESTROY], 0);
        break;
    }
  }
}

gboolean
openvr_overlay_set_mouse_scale (OpenVROverlay *self, float width, float height)
{
  GET_OVERLAY_FUNCTIONS

  struct HmdVector2_t mouse_scale = {{ width, height }};
  err = f->SetOverlayMouseScale (self->overlay_handle, &mouse_scale);

  OVERLAY_CHECK_ERROR ("SetOverlayMouseScale", err);
  return TRUE;
}

/*
 * Sets render model to draw behind this overlay and the vertex color to use,
 * pass NULL for color to match the overlays vertex color
 */

gboolean
openvr_overlay_set_model (OpenVROverlay *self, gchar *name,
                          struct HmdColor_t *color)
{
  GET_OVERLAY_FUNCTIONS

  err = f->SetOverlayRenderModel (self->overlay_handle, name, color);

  OVERLAY_CHECK_ERROR ("SetOverlayRenderModel", err);
  return TRUE;
}

gboolean
openvr_overlay_get_model (OpenVROverlay *self, gchar *name,
                          struct HmdColor_t *color, uint32_t *id)
{
  GET_OVERLAY_FUNCTIONS

  *id = f->GetOverlayRenderModel (self->overlay_handle,
                                  name, k_unMaxPropertyStringSize,
                                  color, &err);

  OVERLAY_CHECK_ERROR ("GetOverlayRenderModel", err);
  return TRUE;
}

gboolean
openvr_overlay_clear_texture (OpenVROverlay *self)
{
  GET_OVERLAY_FUNCTIONS

  err = f->ClearOverlayTexture (self->overlay_handle);

  OVERLAY_CHECK_ERROR ("ClearOverlayTexture", err);
  return TRUE;
}

gboolean
openvr_overlay_get_color (OpenVROverlay *self, graphene_vec3_t *color)
{
  GET_OVERLAY_FUNCTIONS

  float r, g, b;
  err = f->GetOverlayColor (self->overlay_handle, &r, &g, &b);

  OVERLAY_CHECK_ERROR ("GetOverlayColor", err);

  graphene_vec3_init (color, r, g, b);

  return TRUE;
}

gboolean
openvr_overlay_set_color (OpenVROverlay *self, const graphene_vec3_t *color)
{
  GET_OVERLAY_FUNCTIONS

  err = f->SetOverlayColor (self->overlay_handle,
                            graphene_vec3_get_x (color),
                            graphene_vec3_get_y (color),
                            graphene_vec3_get_z (color));

  OVERLAY_CHECK_ERROR ("SetOverlayColor", err);
  return TRUE;
}

gboolean
openvr_overlay_set_alpha (OpenVROverlay *self, float alpha)
{
  GET_OVERLAY_FUNCTIONS

  err = f->SetOverlayAlpha (self->overlay_handle, alpha);

  OVERLAY_CHECK_ERROR ("SetOverlayAlpha", err);
  return TRUE;
}

gboolean
openvr_overlay_set_width_meters (OpenVROverlay *self, float meters)
{
  GET_OVERLAY_FUNCTIONS

  err = f->SetOverlayWidthInMeters (self->overlay_handle, meters);

  OVERLAY_CHECK_ERROR ("SetOverlayWidthInMeters", err);
  return TRUE;
}

gboolean
openvr_overlay_set_transform_absolute (OpenVROverlay *self,
                                       graphene_matrix_t *mat)
{
  GET_OVERLAY_FUNCTIONS

  HmdMatrix34_t translation34;
  openvr_math_graphene_to_matrix34 (mat, &translation34);

  err = f->SetOverlayTransformAbsolute (self->overlay_handle,
                                        context->origin, &translation34);

  OVERLAY_CHECK_ERROR ("SetOverlayTransformAbsolute", err);
  return TRUE;
}

gboolean
openvr_overlay_get_transform_absolute (OpenVROverlay *self,
                                       graphene_matrix_t *mat)
{
  GET_OVERLAY_FUNCTIONS

  HmdMatrix34_t translation34;

  err = f->GetOverlayTransformAbsolute (self->overlay_handle,
                                       &context->origin,
                                       &translation34);

  openvr_math_matrix34_to_graphene (&translation34, mat);

  OVERLAY_CHECK_ERROR ("GetOverlayTransformAbsolute", err);
  return TRUE;
}

gboolean
openvr_overlay_intersects (OpenVROverlay      *overlay,
                           graphene_point3d_t *intersection_point,
                           graphene_matrix_t  *transform)
{
  OpenVRContext *context = openvr_context_get_instance ();
  VROverlayIntersectionParams_t params = {
    .eOrigin = context->origin
  };

  graphene_vec3_t direction;
  openvr_math_direction_from_matrix (transform, &direction);
  graphene_vec3_to_float (&direction, params.vDirection.v);

  graphene_vec3_t translation;
  openvr_math_matrix_get_translation (transform, &translation);
  graphene_vec3_to_float (&translation, params.vSource.v);

  struct VROverlayIntersectionResults_t results;
  gboolean intersects = context->overlay->ComputeOverlayIntersection (
      overlay->overlay_handle, &params, &results);

  intersection_point->x = results.vPoint.v[0];
  intersection_point->y = results.vPoint.v[1];
  intersection_point->z = results.vPoint.v[2];

  return intersects;
}


gboolean
openvr_overlay_set_gdk_pixbuf_raw (OpenVROverlay *self, GdkPixbuf * pixbuf)
{
  int width = gdk_pixbuf_get_width (pixbuf);
  int height = gdk_pixbuf_get_height (pixbuf);
  guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);

  uint32_t depth = 3;
  switch (gdk_pixbuf_get_n_channels (pixbuf))
  {
    case 3:
      depth = 3;
      break;
    case 4:
      depth = 4;
      break;
    default:
      depth = 3;
  }

  return openvr_overlay_set_raw (self, pixels, width, height, depth);
}


gboolean
openvr_overlay_set_raw (OpenVROverlay *self, guchar *pixels,
                        uint32_t width, uint32_t height, uint32_t depth)
{
  GET_OVERLAY_FUNCTIONS

  err = f->SetOverlayRaw (self->overlay_handle,
                          (void*) pixels, width, height, depth);

  OVERLAY_CHECK_ERROR ("SetOverlayRaw", err);
  return TRUE;
}

gboolean
openvr_overlay_get_size_pixels (OpenVROverlay *self, PixelSize *size)
{
  GET_OVERLAY_FUNCTIONS

  err =  f->GetOverlayTextureSize (self->overlay_handle,
                                   &size->width, &size->height);

  OVERLAY_CHECK_ERROR ("GetOverlayTextureSize", err);

  return TRUE;
}

gboolean
openvr_overlay_get_width_meters (OpenVROverlay *self, float *width)
{
  GET_OVERLAY_FUNCTIONS

  err = f->GetOverlayWidthInMeters (self->overlay_handle, width);

  OVERLAY_CHECK_ERROR ("GetOverlayWidthInMeters", err);

  return TRUE;
}

/*
 * There is no function to get the height or aspect ratio of an overlay,
 * so we need to calculate it from width + texture size
 * the texture aspect ratio should be preserved.
 */

gboolean
openvr_overlay_get_size_meters (OpenVROverlay *self, graphene_vec2_t *size)
{
  gfloat width_meters;
  if (!openvr_overlay_get_width_meters (self, &width_meters))
    return FALSE;

  PixelSize size_pixels = {};
  if (!openvr_overlay_get_size_pixels (self, &size_pixels))
    return FALSE;

  gfloat aspect = (gfloat) size_pixels.width / (gfloat) size_pixels.height;
  gfloat height_meters = width_meters / aspect;

  graphene_vec2_init (size, width_meters, height_meters);

  return TRUE;
}

/*
 * The transformation matrix describes the *center* point of an overlay
 * to calculate 2D coordinates relative to overlay origin we have to shift.
 *
 * To calculate the position in the overlay in any orientation, we can invert
 * the transformation matrix of the overlay. This transformation matrix would
 * bring the center of our overlay into the origin of the coordinate system,
 * facing us (+z), the overlay being in the xy plane (since by convention that
 * is the neutral position for overlays).
 *
 * Since the transformation matrix transforms every possible point on the
 * overlay onto the same overlay as it is in the origin in the xy plane,
 * it transforms in particular the intersection point onto its position on the
 * overlay in the xy plane.
 * Then we only need to shift it by half of the overlay widht/height, because
 * the *center* of the overlay sits in the origin.
 */

gboolean
openvr_overlay_get_2d_intersection (OpenVROverlay      *overlay,
                                    graphene_point3d_t *intersection_point,
                                    PixelSize          *size_pixels,
                                    graphene_point_t   *position_2d)
{
  /* transform intersection point to origin */
  graphene_matrix_t transform;
  openvr_overlay_get_transform_absolute (overlay, &transform);

  graphene_matrix_t inverse_transform;
  graphene_matrix_inverse (&transform, &inverse_transform);

  graphene_point3d_t intersection_origin;
  graphene_matrix_transform_point3d (&inverse_transform,
                                      intersection_point,
                                     &intersection_origin);

  graphene_vec2_t position_2d_vec;
  graphene_vec2_init (&position_2d_vec,
                      intersection_origin.x,
                      intersection_origin.y);

  /* normalize coordinates to [0 - 1, 0 - 1] */
  graphene_vec2_t size_meters;
  if (!openvr_overlay_get_size_meters (overlay, &size_meters))
    return FALSE;

  graphene_vec2_divide (&position_2d_vec, &size_meters, &position_2d_vec);

  /* move origin from cetner to corner of overlay */
  graphene_vec2_t center_normalized;
  graphene_vec2_init (&center_normalized, 0.5f, 0.5f);

  graphene_vec2_add (&position_2d_vec, &center_normalized, &position_2d_vec);

  /* invert y axis */
  graphene_vec2_init (&position_2d_vec,
                      graphene_vec2_get_x (&position_2d_vec),
                      1.0f - graphene_vec2_get_y (&position_2d_vec));

  /* scale to pixel coordinates */
  graphene_vec2_t size_pixels_vec;
  graphene_vec2_init (&size_pixels_vec,
                      size_pixels->width,
                      size_pixels->height);

  graphene_vec2_multiply (&position_2d_vec, &size_pixels_vec, &position_2d_vec);

  /* return point_t */
  graphene_point_init_from_vec2 (position_2d, &position_2d_vec);

  return TRUE;
}

void
openvr_overlay_poll_3d_intersection (OpenVROverlay      *self,
                                     graphene_matrix_t  *pose)
{
  OpenVRIntersectionEvent *event = malloc (sizeof (OpenVRIntersectionEvent));

  gboolean intersects = openvr_overlay_intersects (self,
                                                   &event->intersection_point,
                                                   pose);

  graphene_matrix_init_from_matrix (&event->transform, pose);

  event->has_intersection = intersects;

  g_signal_emit (self, overlay_signals[INTERSECTION_EVENT], 0, event);
}
