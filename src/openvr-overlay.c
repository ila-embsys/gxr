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

guint overlay_signals[LAST_SIGNAL] = { 0 };

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

  overlay_signals[MOTION_NOTIFY_EVENT3D] =
    g_signal_new ("motion-notify-event-3d",
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
openvr_overlay_create_width (OpenVROverlay *self,
                             gchar* key,
                             gchar* name,
                             float meters)
{
  GET_OVERLAY_FUNCTIONS

  err = f->CreateOverlay (key, name, &self->overlay_handle);

  OVERLAY_CHECK_ERROR ("CreateOverlay", err);

  f->SetOverlayWidthInMeters (self->overlay_handle, meters);
  f->SetOverlayInputMethod (self->overlay_handle,
                            VROverlayInputMethod_Mouse);
  return TRUE;

}

gboolean
openvr_overlay_create (OpenVROverlay *self,
                       gchar* key,
                       gchar* name)
{
  return openvr_overlay_create_width (self, key, name, 1.5);
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

  if (!openvr_overlay_set_width_meters (self, 1.5f))
    return FALSE;

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
