/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_OVERLAY_H_
#define OPENVR_GLIB_OVERLAY_H_

#include <stdint.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_OVERLAY openvr_overlay_get_type()
G_DECLARE_FINAL_TYPE (OpenVROverlay, openvr_overlay, OPENVR, OVERLAY, GObject)

struct _OpenVROverlay
{
  GObjectClass parent_class;

  uint64_t overlay_handle;
  uint64_t thumbnail_handle;
};

OpenVROverlay *openvr_overlay_new (void);

void openvr_overlay_create (OpenVROverlay *self,
                            const gchar* key,
                            const gchar* name);

void openvr_overlay_upload_gdk_pixbuf (OpenVROverlay *self,
                                       GdkPixbuf * pixbuf);

void openvr_overlay_poll_event (OpenVROverlay *self);

void openvr_overlay_set_mouse_scale (OpenVROverlay *self,
                                     float width,
                                     float height);

G_END_DECLS

#endif /* OPENVR_GLIB_OVERLAY_H_ */
