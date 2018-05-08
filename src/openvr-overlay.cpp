/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#define GL_GLEXT_PROTOTYPES 1
#include <GL/glcorearb.h>

#include "openvr-overlay.h"
#include <openvr.h>

G_DEFINE_TYPE (OpenVROverlay, openvr_overlay, G_TYPE_OBJECT)

static void
openvr_overlay_class_init (OpenVROverlayClass *klass)
{}

static void
openvr_overlay_init (OpenVROverlay *self)
{}

OpenVROverlay *
openvr_overlay_new (void)
{
  return static_cast<OpenVROverlay*> (g_object_new (OPENVR_TYPE_OVERLAY, 0));
}

void openvr_overlay_create (OpenVROverlay *self,
                            const gchar* key,
                            const gchar* name)
{
  gboolean ret = FALSE;

  if (vr::VROverlay()) {
    vr::VROverlayError overlayError =
      vr::VROverlay()->CreateDashboardOverlay (key, name,
                                               &self->overlay_handle,
                                               &self->thumbnail_handle);
    ret = overlayError == vr::VROverlayError_None;
  }

  if (ret) {
    vr::VROverlay()->SetOverlayWidthInMeters (self->overlay_handle, 1.5f);
    vr::VROverlay()->SetOverlayInputMethod (self->overlay_handle,
                                            vr::VROverlayInputMethod_Mouse);
  }
}

void openvr_overlay_upload_gdk_pixbuf (OpenVROverlay *self,
                                       GdkPixbuf * pixbuf)
{
  // skip rendering if the overlay isn't visible
  if ((self->overlay_handle == vr::k_ulOverlayHandleInvalid) || !vr::VROverlay () ||
      (!vr::VROverlay()->IsOverlayVisible (self->overlay_handle) &&
       !vr::VROverlay()->IsOverlayVisible (self->thumbnail_handle)))
    return;

  GLuint tex;
  glGenTextures (1, &tex);
  glBindTexture (GL_TEXTURE_2D, tex);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  int width = gdk_pixbuf_get_width (pixbuf);
  int height = gdk_pixbuf_get_height (pixbuf);
  guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);

  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height,
                0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

  if (tex != 0) {
    vr::Texture_t texture = {
      reinterpret_cast<void*> (tex),
      vr::TextureType_OpenGL,
      vr::ColorSpace_Auto
    };
    vr::VROverlay ()->SetOverlayTexture (self->overlay_handle, &texture);
  }
}

void openvr_overlay_query_events (OpenVROverlay *self, GdkPixbuf * pixbuf)
{
  if (!vr::VRSystem ()) return;

  vr::VREvent_t vrEvent;
  while (vr::VROverlay()->PollNextOverlayEvent (self->overlay_handle,
                                                &vrEvent,
                                                sizeof (vrEvent)))
  {
    switch (vrEvent.eventType)
    {
      case vr::VREvent_MouseMove:
      {
        g_print("received mouse move event [%f %f]\n",
                vrEvent.data.mouse.x, vrEvent.data.mouse.y);
      } break;

      case vr::VREvent_MouseButtonDown:
      {
        gboolean is_right_button =
          vrEvent.data.mouse.button == vr::VRMouseButton_Right;
        printf("Event: VREvent_MouseButtonDown %s\n",
          is_right_button ? "Right" : "Left");
      } break;

      case vr::VREvent_MouseButtonUp:
      {
        gboolean is_right_button =
          vrEvent.data.mouse.button == vr::VRMouseButton_Right;
        printf("Event: VREvent_MouseButtonUp %s\n",
          is_right_button ? "Right" : "Left");
      } break;

      case vr::VREvent_OverlayShown:
      {
        printf("Event: VREvent_OverlayShown\n");
        openvr_overlay_upload_gdk_pixbuf (self, pixbuf);
      } break;

      case vr::VREvent_Quit:
        printf("Event: VREvent_Quit\n");
        // TODO: quit main loop here
        break;
    }
  }

  if (self->thumbnail_handle != vr::k_ulOverlayHandleInvalid) {
    while (vr::VROverlay()->PollNextOverlayEvent(self->thumbnail_handle,
                                                 &vrEvent, sizeof (vrEvent)))
      {
      switch (vrEvent.eventType) {
        case vr::VREvent_OverlayShown:
          break;
      }
    }
  }
}

void openvr_overlay_set_mouse_scale (OpenVROverlay *self, GdkPixbuf * pixbuf)
{
  if (vr::VROverlay ()) {
    vr::HmdVector2_t mouse_scale = {
      static_cast<float> (gdk_pixbuf_get_width (pixbuf)),
      static_cast<float> (gdk_pixbuf_get_height (pixbuf))
    };
    vr::VROverlay ()->SetOverlayMouseScale (self->overlay_handle, &mouse_scale);
  }
}
