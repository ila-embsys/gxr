/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_OVERLAY_H_
#define OPENVR_GLIB_OVERLAY_H_

#define COGL_ENABLE_EXPERIMENTAL_API 1
#include <cogl/cogl.h>

#include <stdint.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>

#include <openvr_capi.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_OVERLAY openvr_overlay_get_type()
G_DECLARE_FINAL_TYPE (OpenVROverlay, openvr_overlay, OPENVR, OVERLAY, GObject)

struct _OpenVROverlay
{
  GObjectClass parent_class;

  VROverlayHandle_t overlay_handle;
  VROverlayHandle_t thumbnail_handle;
  struct VR_IVROverlay_FnTable *functions;
};

OpenVROverlay *openvr_overlay_new (void);

void openvr_overlay_create (OpenVROverlay *self,
                            gchar* key,
                            gchar* name);

void
openvr_overlay_create_for_dashboard (OpenVROverlay *self,
                                     gchar* key,
                                     gchar* name);

void openvr_overlay_upload_gdk_pixbuf (OpenVROverlay *self,
                                       GdkPixbuf * pixbuf);

void openvr_overlay_upload_cairo_surface (OpenVROverlay *self,
                                          cairo_surface_t* surface);


void
openvr_overlay_upload_cogl (OpenVROverlay *self, CoglTexture *cogl_texture);

void openvr_overlay_poll_event (OpenVROverlay *self);

void openvr_overlay_set_mouse_scale (OpenVROverlay *self,
                                     float width,
                                     float height);

void
openvr_overlay_upload_pixels (OpenVROverlay *self,
                              guchar *pixels,
                              int width,
                              int height,
                              unsigned gl_format);

void
openvr_overlay_set_raw (OpenVROverlay *self, guchar *pixels,
                        uint32_t width, uint32_t height, uint32_t depth);

void
openvr_overlay_set_cairo_surface_raw (OpenVROverlay *self,
                                      cairo_surface_t* surface);

void
openvr_overlay_set_gl_texture (OpenVROverlay *self, unsigned tex);

gboolean openvr_overlay_is_valid (OpenVROverlay *self);
gboolean openvr_overlay_is_visible (OpenVROverlay *self);
gboolean openvr_overlay_thumbnail_is_visible (OpenVROverlay *self);

gboolean openvr_overlay_is_available (OpenVROverlay * self);

void
openvr_overlay_set_gdk_pixbuf_raw (OpenVROverlay *self,
                                   GdkPixbuf * pixbuf);

void
openvr_overlay_upload_pixels_cogl (OpenVROverlay *self,
                                   guchar *pixels,
                                   int width,
                                   int height,
                                   unsigned gl_format);

G_END_DECLS

#endif /* OPENVR_GLIB_OVERLAY_H_ */
