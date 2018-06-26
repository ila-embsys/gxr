/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <time.h>
#include <glib/gprintf.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/glcorearb.h>

#include <gdk/gdk.h>

#include "openvr-system.h"
#include "openvr-overlay.h"
#include <openvr_capi.h>
#include "openvr_capi_global.h"

#include "openvr-global.h"

G_DEFINE_TYPE (OpenVROverlay, openvr_overlay, G_TYPE_OBJECT)

enum {
  MOTION_NOTIFY_EVENT,
  BUTTON_PRESS_EVENT,
  BUTTON_RELEASE_EVENT,
  SHOW,
  DESTROY,
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
}

gboolean _overlay_init_fn_table (OpenVROverlay *self)
{
  INIT_FN_TABLE (self->functions, Overlay);
}

static void
openvr_overlay_init (OpenVROverlay *self)
{
  self->overlay_handle = 0;
  self->thumbnail_handle = 0;

  _overlay_init_fn_table (self);
}

OpenVROverlay *
openvr_overlay_new (void)
{
  return (OpenVROverlay*) g_object_new (OPENVR_TYPE_OVERLAY, 0);
}

void openvr_overlay_create (OpenVROverlay *self,
                            gchar* key,
                            gchar* name)
{
  EVROverlayError err = self->functions->CreateDashboardOverlay(
    key, name, &self->overlay_handle, &self->thumbnail_handle);

  if (err != EVROverlayError_VROverlayError_None)
    {
      g_printerr ("Could not create overlay: %s\n",
                  self->functions->GetOverlayErrorNameFromEnum (err));
    }
  else
    {
      self->functions->SetOverlayWidthInMeters (self->overlay_handle, 1.5f);
      self->functions->SetOverlayInputMethod (self->overlay_handle,
                                              VROverlayInputMethod_Mouse);
    }
}

gboolean openvr_overlay_is_valid (OpenVROverlay *self)
{
  return self->overlay_handle != k_ulOverlayHandleInvalid;
}

gboolean openvr_overlay_is_visible (OpenVROverlay *self)
{
  return self->functions->IsOverlayVisible (self->overlay_handle);
}

gboolean openvr_overlay_thumbnail_is_visible (OpenVROverlay *self)
{
  return self->functions->IsOverlayVisible (self->thumbnail_handle);
}

void
openvr_overlay_set_gdk_pixbuf_raw (OpenVROverlay *self,
                                   GdkPixbuf * pixbuf)
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

  g_print ("openvr_overlay_set_gdk_pixbuf_raw %dx%dx%d\n",
           width, height, depth);

  openvr_overlay_set_raw (self, pixels,
                          width,
                          height,
                          depth);
}

void
openvr_overlay_upload_gdk_pixbuf (OpenVROverlay *self,
                                  GdkPixbuf * pixbuf)
{
  int width = gdk_pixbuf_get_width (pixbuf);
  int height = gdk_pixbuf_get_height (pixbuf);
  guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);

  GLenum gl_format;
  switch (gdk_pixbuf_get_n_channels (pixbuf))
  {
    case 3:
      gl_format = GL_RGB;
      break;
    case 4:
      gl_format = GL_RGBA;
      break;
    default:
      gl_format = GL_RGBA;
  }

  openvr_overlay_upload_pixels (self, pixels, width, height, gl_format);
}

void
openvr_overlay_upload_cogl (OpenVROverlay *self, CoglTexture *cogl_texture)
{
  GLuint tex;
  GLenum target;

  g_assert (cogl_is_texture (cogl_texture));
  CoglBool ret = cogl_texture_get_gl_texture (cogl_texture, &tex, &target);

  switch (target)
    {
    case GL_TEXTURE_1D:
      g_print ("GL_TEXTURE_1D texture target\n");
      break;
    case GL_TEXTURE_2D:
      g_print ("GL_TEXTURE_2D texture target\n");
      break;
    case GL_TEXTURE_3D:
      g_print ("GL_TEXTURE_3D texture target\n");
      break;
    case GL_TEXTURE_RECTANGLE:
      g_print ("GL_TEXTURE_RECTANGLE texture target\n");
      break;
    default:
      g_print ("Unknown texture target\n");
    }

  if (!ret)
    {
      g_printerr("Getting texture handle failed.\n");
    }

  openvr_overlay_set_gl_texture (self, tex);
}

void
openvr_overlay_set_gl_texture (OpenVROverlay *self, GLuint tex)
{
  if (tex != 0)
    {
      struct Texture_t texture = {
        (void*)(uintptr_t) tex,
        ETextureType_TextureType_OpenGL,
        EColorSpace_ColorSpace_Auto
      };
      EVROverlayError err =
        self->functions->SetOverlayTexture (self->overlay_handle, &texture);

      if (err != EVROverlayError_VROverlayError_None)
        {
          g_printerr ("Could not set overlay texture: %s\n",
                      self->functions->GetOverlayErrorNameFromEnum (err));
        }
    }
}

void
openvr_overlay_set_raw (OpenVROverlay *self, guchar *pixels,
                        uint32_t width, uint32_t height, uint32_t depth)
{
  EVROverlayError err = self->functions->SetOverlayRaw (
    self->overlay_handle, (void*) pixels, width, height, depth);

  if (err != EVROverlayError_VROverlayError_None)
    {
      g_printerr ("Could not set overlay texture: %s\n",
                  self->functions->GetOverlayErrorNameFromEnum (err));
    }

}

void
openvr_overlay_upload_cairo_surface (OpenVROverlay *self,
                                     cairo_surface_t* surface)
{
  int width = cairo_image_surface_get_width (surface);
  int height = cairo_image_surface_get_height (surface);

  cairo_format_t cr_format = cairo_image_surface_get_format (surface);

  GLenum gl_format = GL_RGB;
  switch (cr_format)
  {
  case CAIRO_FORMAT_INVALID:
    g_print("CAIRO_FORMAT_INVALID\n");
    break;
  case CAIRO_FORMAT_ARGB32:
    g_print("CAIRO_FORMAT_ARGB32\n");
    gl_format = GL_BGRA;
    break;
  case CAIRO_FORMAT_RGB24:
    g_print("CAIRO_FORMAT_RGB24\n");
    break;
  case CAIRO_FORMAT_A8:
    g_print("CAIRO_FORMAT_A8\n");
    break;
  case CAIRO_FORMAT_A1:
    g_print("CAIRO_FORMAT_A1\n");
    break;
  case CAIRO_FORMAT_RGB16_565:
    g_print("CAIRO_FORMAT_RGB16_565\n");
    break;
  case CAIRO_FORMAT_RGB30:
    g_print("CAIRO_FORMAT_RGB30\n");
    break;
  default:
    g_print("Unknown format\n");
  }

  guchar *pixels = cairo_image_surface_get_data (surface);

  openvr_overlay_upload_pixels_cogl (self, pixels, width, height, gl_format);
}

void
openvr_overlay_set_cairo_surface_raw (OpenVROverlay *self,
                                      cairo_surface_t* surface)
{
  int width = cairo_image_surface_get_width (surface);
  int height = cairo_image_surface_get_height (surface);

  cairo_format_t cr_format = cairo_image_surface_get_format (surface);

  uint32_t depth = 3;
  switch (cr_format)
  {
  case CAIRO_FORMAT_INVALID:
    g_print("CAIRO_FORMAT_INVALID\n");
    break;
  case CAIRO_FORMAT_ARGB32:
    g_print("CAIRO_FORMAT_ARGB32\n");
    depth = 4;
    break;
  case CAIRO_FORMAT_RGB24:
    g_print("CAIRO_FORMAT_RGB24\n");
    break;
  case CAIRO_FORMAT_A8:
    g_print("CAIRO_FORMAT_A8\n");
    break;
  case CAIRO_FORMAT_A1:
    g_print("CAIRO_FORMAT_A1\n");
    break;
  case CAIRO_FORMAT_RGB16_565:
    g_print("CAIRO_FORMAT_RGB16_565\n");
    break;
  case CAIRO_FORMAT_RGB30:
    g_print("CAIRO_FORMAT_RGB30\n");
    break;
  default:
    g_print("Unknown format\n");
  }

  guchar *pixels = cairo_image_surface_get_data (surface);

  openvr_overlay_set_raw (self, pixels, width, height, depth);
}



static gboolean
load_gl_symbol (const char  *name,
                void       **func)
{
  *func = cogl_get_proc_address (name);
  if (!*func)
    {
      g_printerr ("Failed to resolve required GL symbol \"%s\"\n", name);
      return FALSE;
    }
  return TRUE;
}


static void      (*meta_gl_gen_textures) (GLsizei n, GLuint * textures);
static void      (*meta_gl_bin_texture) (GLenum target, GLuint texture);
static void      (*meta_gl_pixel_store_i) (GLenum pname, GLint param);
static void      (*meta_gl_tex_image_2d) (GLenum target,
                                          GLint level,
                                          GLint internalFormat,
                                          GLsizei width,
                                          GLsizei height,
                                          GLint border,
                                          GLenum format,
                                          GLenum type,
                                          const GLvoid * data);

void
openvr_overlay_upload_pixels_cogl (OpenVROverlay *self,
                                   guchar *pixels,
                                   int width,
                                   int height,
                                   GLenum gl_format)
{
  if (!load_gl_symbol ("glGenTextures", (void **) &meta_gl_gen_textures))
    g_printerr ("Could not load symbol glGenTextures.\n");

  if (!load_gl_symbol ("glBindTexture", (void **) &meta_gl_bin_texture))
    g_printerr ("Could not load symbol glGenTextures.\n");

  if (!load_gl_symbol ("glPixelStorei", (void **) &meta_gl_pixel_store_i))
    g_printerr ("Could not load symbol glGenTextures.\n");

  if (!load_gl_symbol ("glTexImage2D", (void **) &meta_gl_tex_image_2d))
    g_printerr ("Could not load symbol glGenTextures.\n");

  GLuint tex;
  meta_gl_gen_textures (1, &tex);
  meta_gl_bin_texture (GL_TEXTURE_2D, tex);

  /* cogl context needs GL_UNPACK_ROW_LENGTH */
  meta_gl_pixel_store_i (GL_UNPACK_ROW_LENGTH, width);

  /* CoGL Default parameters */
  /*
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
  */
  meta_gl_tex_image_2d (GL_TEXTURE_2D, 0, GL_RGB, width, height,
                        0, gl_format, GL_UNSIGNED_BYTE, pixels);

  openvr_overlay_set_gl_texture (self, tex);
}

void
openvr_overlay_upload_pixels (OpenVROverlay *self,
                              guchar *pixels,
                              int width,
                              int height,
                              GLenum gl_format)
{
  GLuint tex;
  glGenTextures (1, &tex);
  glBindTexture (GL_TEXTURE_2D, tex);

  /* cogl context needs GL_UNPACK_ROW_LENGTH */
  glPixelStorei(GL_UNPACK_ROW_LENGTH, width);

  /* CoGL Default parameters */
  /*
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
  */
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height,
                0, gl_format, GL_UNSIGNED_BYTE, pixels);

  openvr_overlay_set_gl_texture (self, tex);
}

#define SEC_IN_NSEC_D 1000000000.0
#define SEC_IN_MSEC_D 1000.0
#define SEC_IN_NSEC_L 1000000000L

void
_float_seconds_to_timespec (float in, struct timespec* out)
{
  out->tv_sec = (time_t) in;
  out->tv_nsec = (long) ((in - (float) out->tv_sec) * SEC_IN_NSEC_D);
}

/* assuming a > b */
void
_substract_timespecs (struct timespec* a,
                      struct timespec* b,
                      struct timespec* out)
{
  out->tv_sec = a->tv_sec - b->tv_sec;

  if (a->tv_nsec < b->tv_nsec)
  {
    out->tv_nsec = a->tv_nsec + SEC_IN_NSEC_L - b->tv_nsec;
    out->tv_sec--;
  }
  else
  {
    out->tv_nsec = a->tv_nsec - b->tv_nsec;
  }
}

double
_timespec_to_double_s (struct timespec* time)
{
  return ((double) time->tv_sec + (time->tv_nsec / SEC_IN_NSEC_D));
}

guint32
_age_s_to_monotonic_ms (float age)
{
  struct timespec mono_time;
  if (clock_gettime (CLOCK_MONOTONIC, &mono_time) != 0)
  {
    fprintf (stderr, "Could not read system clock\n");
    return 0;
  }

  struct timespec event_age;
  _float_seconds_to_timespec (age, &event_age);

  struct timespec event_age_on_mono;
  _substract_timespecs (&mono_time, &event_age, &event_age_on_mono);

  double time_s = _timespec_to_double_s (&event_age_on_mono);

  return (guint32) (time_s * SEC_IN_MSEC_D);
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

  while (self->functions->PollNextOverlayEvent (self->overlay_handle,
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
        event->motion.time = _age_s_to_monotonic_ms (vr_event.eventAgeSeconds);
        g_signal_emit (self, overlay_signals[MOTION_NOTIFY_EVENT], 0, event);
      } break;

      case EVREventType_VREvent_MouseButtonDown:
      {
        GdkEvent *event = gdk_event_new (GDK_BUTTON_PRESS);
        event->button.x = vr_event.data.mouse.x;
        event->button.y = vr_event.data.mouse.y;
        event->button.time = _age_s_to_monotonic_ms (vr_event.eventAgeSeconds);
        event->button.button =
          _vr_to_gdk_mouse_button (vr_event.data.mouse.button);
        g_signal_emit (self, overlay_signals[BUTTON_PRESS_EVENT], 0, event);
      } break;

      case EVREventType_VREvent_MouseButtonUp:
      {
        GdkEvent *event = gdk_event_new (GDK_BUTTON_RELEASE);
        event->button.x = vr_event.data.mouse.x;
        event->button.y = vr_event.data.mouse.y;
        event->button.time = _age_s_to_monotonic_ms (vr_event.eventAgeSeconds);
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

void
openvr_overlay_set_mouse_scale (OpenVROverlay *self, float width, float height)
{
  if (self->functions) {
    struct HmdVector2_t mouse_scale = {{ width, height }};
    self->functions->SetOverlayMouseScale (self->overlay_handle, &mouse_scale);
  }
}

gboolean
openvr_overlay_is_available (OpenVROverlay * self)
{
  return self->functions != NULL;
}
