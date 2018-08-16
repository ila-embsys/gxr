/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_OVERLAY_H_
#define OPENVR_GLIB_OVERLAY_H_

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

void openvr_overlay_poll_event (OpenVROverlay *self);

void openvr_overlay_set_mouse_scale (OpenVROverlay *self,
                                     float width,
                                     float height);

gboolean openvr_overlay_is_valid (OpenVROverlay *self);
gboolean openvr_overlay_is_visible (OpenVROverlay *self);
gboolean openvr_overlay_thumbnail_is_visible (OpenVROverlay *self);

gboolean openvr_overlay_is_available (OpenVROverlay * self);

G_END_DECLS

#endif /* OPENVR_GLIB_OVERLAY_H_ */
