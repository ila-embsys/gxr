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

#include "openvr-time.h"
#include "openvr-math.h"

#define GET_OVERLAY_FUNCTIONS \
  EVROverlayError err; \
  OpenVRContext *context = openvr_context_get_instance (); \
  struct VR_IVROverlay_FnTable *f = context->overlay;

#define OVERLAY_CHECK_ERROR(fun, res) \
{ \
  if (res != EVROverlayError_VROverlayError_None) \
    { \
      g_printerr ("ERROR: " fun ": failed with %s in %s:%d\n", \
                  f->GetOverlayErrorNameFromEnum (res), __FILE__, __LINE__); \
      return FALSE; \
    } \
}

typedef struct _OpenVROverlayPrivate
{
  GObjectClass parent_class;

  VROverlayHandle_t overlay_handle;
  VROverlayHandle_t thumbnail_handle;
  gboolean      flip_y;

} OpenVROverlayPrivate;

static struct VRTextureBounds_t defaultBounds = { 0., 0., 1., 1. };
static struct VRTextureBounds_t flippedBounds = { 0., 1., 1., 0. };

G_DEFINE_TYPE_WITH_PRIVATE (OpenVROverlay, openvr_overlay, G_TYPE_OBJECT)

enum {
  MOTION_NOTIFY_EVENT,
  BUTTON_PRESS_EVENT,
  BUTTON_RELEASE_EVENT,
  SHOW,
  HIDE,
  DESTROY,
  KEYBOARD_PRESS_EVENT,
  KEYBOARD_CLOSE_EVENT,
  LAST_SIGNAL
};

static guint overlay_signals[LAST_SIGNAL] = { 0 };

static void
openvr_overlay_finalize (GObject *gobject);

static void
openvr_overlay_class_init (OpenVROverlayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = openvr_overlay_finalize;
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
                  (GSignalFlags)
                     (G_SIGNAL_RUN_CLEANUP |
                      G_SIGNAL_NO_RECURSE |
                      G_SIGNAL_NO_HOOKS),
                   0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  overlay_signals[KEYBOARD_PRESS_EVENT] =
    g_signal_new ("keyboard-press-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  overlay_signals[KEYBOARD_CLOSE_EVENT] =
    g_signal_new ("keyboard-close-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void
openvr_overlay_init (OpenVROverlay *self)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);
  priv->overlay_handle = 0;
  priv->thumbnail_handle = 0;
}

OpenVROverlay *
openvr_overlay_new (void)
{
  return (OpenVROverlay*) g_object_new (OPENVR_TYPE_OVERLAY, 0);
}

gboolean
openvr_overlay_create (OpenVROverlay *self, gchar* key, gchar* name)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  /* k_unVROverlayMaxKeyLength is the limit including the null terminator */
  if (strlen(key) + 1 > k_unVROverlayMaxKeyLength)
    {
      g_printerr ("Overlay key too long, must be shorter than %d characters\n",
                  k_unMaxSettingsKeyLength - 1);
      return FALSE;
    }


  char *name_trimmed = strndup(name, k_unVROverlayMaxNameLength - 1);

  err = f->CreateOverlay (key, name_trimmed, &priv->overlay_handle);

  free (name_trimmed);

  OVERLAY_CHECK_ERROR ("CreateOverlay", err)

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
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  err = f->CreateDashboardOverlay (key, name, &priv->overlay_handle,
                                  &priv->thumbnail_handle);

  OVERLAY_CHECK_ERROR ("CreateDashboardOverlay", err)

  return TRUE;
}

gboolean
openvr_overlay_is_valid (OpenVROverlay *self)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);
  return priv->overlay_handle != k_ulOverlayHandleInvalid;
}

gboolean
openvr_overlay_show (OpenVROverlay *self)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  err = f->ShowOverlay (priv->overlay_handle);

  OVERLAY_CHECK_ERROR ("ShowOverlay", err)
  return TRUE;
}

gboolean
openvr_overlay_hide (OpenVROverlay *self)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  err = f->HideOverlay (priv->overlay_handle);

  OVERLAY_CHECK_ERROR ("HideOverlay", err)
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
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);
  OpenVRContext *context = openvr_context_get_instance ();
  return context->overlay->IsOverlayVisible (priv->overlay_handle);
}

gboolean
openvr_overlay_thumbnail_is_visible (OpenVROverlay *self)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);
  OpenVRContext *context = openvr_context_get_instance ();
  return context->overlay->IsOverlayVisible (priv->thumbnail_handle);
}

gboolean
openvr_overlay_set_sort_order (OpenVROverlay *self, uint32_t sort_order)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  err = f->SetOverlaySortOrder (priv->overlay_handle, sort_order);

  OVERLAY_CHECK_ERROR ("SetOverlaySortOrder", err)
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
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  err = f->SetOverlayInputMethod (priv->overlay_handle,
                                  VROverlayInputMethod_Mouse);

  OVERLAY_CHECK_ERROR ("SetOverlayInputMethod", err)

  return TRUE;
}

void
openvr_overlay_poll_event (OpenVROverlay *self)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  struct VREvent_t vr_event;

  OpenVRContext *context = openvr_context_get_instance ();

  while (context->overlay->PollNextOverlayEvent (priv->overlay_handle,
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
      case EVREventType_VREvent_OverlayHidden:
        g_signal_emit (self, overlay_signals[HIDE], 0);
        break;
      case EVREventType_VREvent_Quit:
        g_signal_emit (self, overlay_signals[DESTROY], 0);
        break;

      // TODO: code duplication with system keyboard in OpenVRContext
      case EVREventType_VREvent_KeyboardCharInput:
      {
        // TODO: https://github.com/ValveSoftware/openvr/issues/289
        char *new_input = (char*) vr_event.data.keyboard.cNewInput;

        int len = 0;
        for (; len < 8 && new_input[len] != 0; len++);

        GdkEvent *event = gdk_event_new (GDK_KEY_PRESS);
        event->key.state = TRUE;
        event->key.string = new_input;
        event->key.length = len;
        g_signal_emit (self, overlay_signals[KEYBOARD_PRESS_EVENT], 0, event);
      } break;

      case EVREventType_VREvent_KeyboardClosed:
      {
        g_signal_emit (self, overlay_signals[KEYBOARD_CLOSE_EVENT], 0);
      } break;
    }
  }
}

gboolean
openvr_overlay_set_mouse_scale (OpenVROverlay *self, float width, float height)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  struct HmdVector2_t mouse_scale = {{ width, height }};
  err = f->SetOverlayMouseScale (priv->overlay_handle, &mouse_scale);

  OVERLAY_CHECK_ERROR ("SetOverlayMouseScale", err)
  return TRUE;
}

gboolean
openvr_overlay_clear_texture (OpenVROverlay *self)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  err = f->ClearOverlayTexture (priv->overlay_handle);

  OVERLAY_CHECK_ERROR ("ClearOverlayTexture", err)
  return TRUE;
}

gboolean
openvr_overlay_get_color (OpenVROverlay *self, graphene_vec3_t *color)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  float r, g, b;
  err = f->GetOverlayColor (priv->overlay_handle, &r, &g, &b);

  OVERLAY_CHECK_ERROR ("GetOverlayColor", err)

  graphene_vec3_init (color, r, g, b);

  return TRUE;
}

gboolean
openvr_overlay_set_color (OpenVROverlay *self, const graphene_vec3_t *color)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  err = f->SetOverlayColor (priv->overlay_handle,
                            graphene_vec3_get_x (color),
                            graphene_vec3_get_y (color),
                            graphene_vec3_get_z (color));

  OVERLAY_CHECK_ERROR ("SetOverlayColor", err)
  return TRUE;
}

gboolean
openvr_overlay_set_alpha (OpenVROverlay *self, float alpha)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  err = f->SetOverlayAlpha (priv->overlay_handle, alpha);

  OVERLAY_CHECK_ERROR ("SetOverlayAlpha", err)
  return TRUE;
}

gboolean
openvr_overlay_set_width_meters (OpenVROverlay *self, float meters)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  err = f->SetOverlayWidthInMeters (priv->overlay_handle, meters);

  OVERLAY_CHECK_ERROR ("SetOverlayWidthInMeters", err)
  return TRUE;
}

gboolean
openvr_overlay_set_transform_absolute (OpenVROverlay *self,
                                       graphene_matrix_t *mat)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  HmdMatrix34_t translation34;
  openvr_math_graphene_to_matrix34 (mat, &translation34);

  err = f->SetOverlayTransformAbsolute (priv->overlay_handle,
                                        context->origin, &translation34);

  OVERLAY_CHECK_ERROR ("SetOverlayTransformAbsolute", err)
  return TRUE;
}

gboolean
openvr_overlay_get_transform_absolute (OpenVROverlay *self,
                                       graphene_matrix_t *mat)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  HmdMatrix34_t translation34;

  err = f->GetOverlayTransformAbsolute (priv->overlay_handle,
                                       &context->origin,
                                       &translation34);

  openvr_math_matrix34_to_graphene (&translation34, mat);

  OVERLAY_CHECK_ERROR ("GetOverlayTransformAbsolute", err)
  return TRUE;
}

gboolean
openvr_overlay_intersects (OpenVROverlay      *self,
                           graphene_point3d_t *intersection_point,
                           graphene_matrix_t  *transform)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

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
      priv->overlay_handle, &params, &results);

  intersection_point->x = results.vPoint.v[0];
  intersection_point->y = results.vPoint.v[1];
  intersection_point->z = results.vPoint.v[2];

  return intersects;
}

gboolean
openvr_overlay_set_gdk_pixbuf_raw (OpenVROverlay *self, GdkPixbuf * pixbuf)
{
  guint width = (guint)gdk_pixbuf_get_width (pixbuf);
  guint height = (guint)gdk_pixbuf_get_height (pixbuf);
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
openvr_overlay_set_cairo_surface_raw (OpenVROverlay   *self,
                                      cairo_surface_t *surface)
{
  guchar *pixels = cairo_image_surface_get_data (surface);

  guint width = (guint)cairo_image_surface_get_width (surface);
  guint height = (guint)cairo_image_surface_get_height (surface);

  uint32_t depth;
  cairo_format_t cr_format = cairo_image_surface_get_format (surface);
  switch (cr_format)
    {
    case CAIRO_FORMAT_ARGB32:
      depth = 4;
      break;
    case CAIRO_FORMAT_RGB24:
      depth = 3;
      break;
    case CAIRO_FORMAT_A8:
    case CAIRO_FORMAT_A1:
    case CAIRO_FORMAT_RGB16_565:
    case CAIRO_FORMAT_RGB30:
      g_printerr ("Unsupported Cairo format\n");
      return FALSE;
    case CAIRO_FORMAT_INVALID:
      g_printerr ("Invalid Cairo format\n");
      return FALSE;
    default:
      g_printerr ("Unknown Cairo format\n");
      return FALSE;
    }

  return openvr_overlay_set_raw (self, pixels, width, height, depth);
}

gboolean
openvr_overlay_set_raw (OpenVROverlay *self, guchar *pixels,
                        uint32_t width, uint32_t height, uint32_t depth)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  err = f->SetOverlayRaw (priv->overlay_handle,
                          (void*) pixels, width, height, depth);

  OVERLAY_CHECK_ERROR ("SetOverlayRaw", err)
  return TRUE;
}

gboolean
openvr_overlay_get_size_pixels (OpenVROverlay *self, OpenVRPixelSize *size)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  err =  f->GetOverlayTextureSize (priv->overlay_handle,
                                   &size->width, &size->height);

  OVERLAY_CHECK_ERROR ("GetOverlayTextureSize", err)

  return TRUE;
}

gboolean
openvr_overlay_get_width_meters (OpenVROverlay *self, float *width)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  err = f->GetOverlayWidthInMeters (priv->overlay_handle, width);

  OVERLAY_CHECK_ERROR ("GetOverlayWidthInMeters", err)

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

  OpenVRPixelSize size_pixels = {};
  if (!openvr_overlay_get_size_pixels (self, &size_pixels))
    return FALSE;

  gfloat aspect = (gfloat) size_pixels.width / (gfloat) size_pixels.height;
  gfloat height_meters = width_meters / aspect;

  graphene_vec2_init (size, width_meters, height_meters);

  return TRUE;
}

gboolean
openvr_overlay_show_keyboard (OpenVROverlay *self)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  err = f->ShowKeyboardForOverlay (
    priv->overlay_handle,
    EGamepadTextInputMode_k_EGamepadTextInputModeNormal,
    EGamepadTextInputLineMode_k_EGamepadTextInputLineModeSingleLine,
    "OpenVR Overlay Keyboard", 1, "", TRUE, 0);

  OVERLAY_CHECK_ERROR ("ShowKeyboardForOverlay", err)

  return TRUE;
}

void
openvr_overlay_set_keyboard_position (OpenVROverlay   *self,
                                      graphene_vec2_t *top_left,
                                      graphene_vec2_t *bottom_right)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS
  (void) err;

  HmdRect2_t rect = {
    .vTopLeft = {
      .v[0] = graphene_vec2_get_x (top_left),
      .v[1] = graphene_vec2_get_y (top_left)
    },
    .vBottomRight = {
      .v[0] = graphene_vec2_get_x (bottom_right),
      .v[1] = graphene_vec2_get_y (bottom_right)
    }
  };
  f->SetKeyboardPositionForOverlay (priv->overlay_handle, rect);
}

gboolean
openvr_overlay_set_translation (OpenVROverlay      *self,
                                graphene_point3d_t *translation)
{
  graphene_matrix_t transform;
  graphene_matrix_init_translate (&transform, translation);
  return openvr_overlay_set_transform_absolute (self, &transform);
}

gboolean
openvr_overlay_destroy (OpenVROverlay *self)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  err = f->DestroyOverlay (priv->overlay_handle);

  OVERLAY_CHECK_ERROR ("DestroyOverlay", err)

  return TRUE;
}

static void
openvr_overlay_finalize (GObject *gobject)
{
  OpenVROverlay *self = OPENVR_OVERLAY (gobject);
  openvr_overlay_destroy (self);
  G_OBJECT_CLASS (openvr_overlay_parent_class)->finalize (gobject);
}

VROverlayHandle_t
openvr_overlay_get_handle (OpenVROverlay *self)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);
  return priv->overlay_handle;
}

gboolean
openvr_overlay_set_model (OpenVROverlay *self,
                          gchar *name,
                          graphene_vec4_t *color)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  struct HmdColor_t hmd_color = {
    .r = graphene_vec4_get_x (color),
    .g = graphene_vec4_get_y (color),
    .b = graphene_vec4_get_z (color),
    .a = graphene_vec4_get_w (color)
  };

  err = f->SetOverlayRenderModel (priv->overlay_handle,
                                  name, &hmd_color);

  OVERLAY_CHECK_ERROR ("SetOverlayRenderModel", err)
  return TRUE;
}

gboolean
openvr_overlay_get_model (OpenVROverlay *self, gchar *name,
                          graphene_vec4_t *color, uint32_t *id)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);

  GET_OVERLAY_FUNCTIONS

  struct HmdColor_t hmd_color;
  *id = f->GetOverlayRenderModel (priv->overlay_handle,
                                  name, k_unMaxPropertyStringSize,
                                  &hmd_color, &err);

  graphene_vec4_init (color,
                      hmd_color.r, hmd_color.g, hmd_color.b, hmd_color.a);

  OVERLAY_CHECK_ERROR ("GetOverlayRenderModel", err)
  return TRUE;
}

void
openvr_overlay_set_flip_y (OpenVROverlay *self,
                           gboolean flip_y)
{
  OpenVROverlayPrivate *priv = openvr_overlay_get_instance_private (self);
  if (flip_y != priv->flip_y)
    {
      OpenVRContext *openvrContext = openvr_context_get_instance();
      VRTextureBounds_t *bounds = flip_y ? &flippedBounds : &defaultBounds;
      openvrContext->overlay->SetOverlayTextureBounds (priv->overlay_handle,
                                                       bounds);

      priv->flip_y = flip_y;
    }
}

/* Submit frame to OpenVR runtime */
bool
openvr_overlay_submit_texture (OpenVROverlay *self,
                               GulkanClient  *client,
                               GulkanTexture *texture)
{
  GET_OVERLAY_FUNCTIONS

  GulkanDevice *device = gulkan_client_get_device (client);

  struct VRVulkanTextureData_t texture_data =
    {
      .m_nImage = (uint64_t)
        (struct VkImage_T *)gulkan_texture_get_image (texture),
      .m_pDevice = gulkan_device_get_handle (device),
      .m_pPhysicalDevice = gulkan_device_get_physical_handle (device),
      .m_pInstance = gulkan_client_get_instance_handle (client),
      .m_pQueue = gulkan_device_get_queue_handle (device),
      .m_nQueueFamilyIndex = gulkan_device_get_queue_family_index (device),
      .m_nWidth = gulkan_texture_get_width (texture),
      .m_nHeight = gulkan_texture_get_height (texture),
      .m_nFormat = gulkan_texture_get_format (texture),
      .m_nSampleCount = 1
    };

  struct Texture_t vr_texture =
    {
      .handle = &texture_data,
      .eType = ETextureType_TextureType_Vulkan,
      .eColorSpace = EColorSpace_ColorSpace_Auto
    };

  VROverlayHandle_t overlay_handle = openvr_overlay_get_handle (self);
  err = f->SetOverlayTexture (overlay_handle, &vr_texture);

  OVERLAY_CHECK_ERROR ("SetOverlayTexture", err)
  return TRUE;
}
