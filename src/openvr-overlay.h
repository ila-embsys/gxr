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
#include <graphene.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_OVERLAY openvr_overlay_get_type()
G_DECLARE_FINAL_TYPE (OpenVROverlay, openvr_overlay, OPENVR, OVERLAY, GObject)

struct _motion_event_3d {
  graphene_matrix_t transform;
  gboolean has_intersection;
  graphene_point3d_t intersection_point;
};

enum {
  MOTION_NOTIFY_EVENT,
  BUTTON_PRESS_EVENT,
  BUTTON_RELEASE_EVENT,
  SHOW,
  DESTROY,
  MOTION_NOTIFY_EVENT3D,
  BUTTON_PRESS_EVENT3D,
  BUTTON_RELEASE_EVENT3D,
  SCROLL_EVENT,
  LAST_SIGNAL
};

extern guint overlay_signals[LAST_SIGNAL];

struct _OpenVROverlay
{
  GObjectClass parent_class;

  VROverlayHandle_t overlay_handle;
  VROverlayHandle_t thumbnail_handle;

  // describes where the overlay is displayed in VR
  // needs to be updated by the application when the overlay is moved
  graphene_matrix_t transform;
};

OpenVROverlay *openvr_overlay_new (void);

gboolean
openvr_overlay_create_width (OpenVROverlay *self,
                             gchar* key,
                             gchar* name,
                             float width_meters);

gboolean
openvr_overlay_create (OpenVROverlay *self,
                       gchar* key,
                       gchar* name);

gboolean
openvr_overlay_create_for_dashboard (OpenVROverlay *self,
                                     gchar* key,
                                     gchar* name);

void
openvr_overlay_poll_event (OpenVROverlay *self);

gboolean
openvr_overlay_set_mouse_scale (OpenVROverlay *self, float width, float height);

gboolean
openvr_overlay_is_valid (OpenVROverlay *self);

gboolean
openvr_overlay_is_visible (OpenVROverlay *self);

gboolean
openvr_overlay_thumbnail_is_visible (OpenVROverlay *self);

gboolean
openvr_overlay_set_visibility (OpenVROverlay *self, gboolean visibility);

gboolean
openvr_overlay_show (OpenVROverlay *self);

gboolean
openvr_overlay_hide (OpenVROverlay *self);

gboolean
openvr_overlay_set_model (OpenVROverlay *self, gchar *name,
                          struct HmdColor_t *color);

gboolean
openvr_overlay_get_model (OpenVROverlay *self, gchar *name,
                          struct HmdColor_t *color, uint32_t *id);

gboolean
openvr_overlay_clear_texture (OpenVROverlay *self);

gboolean
openvr_overlay_get_color (OpenVROverlay *self, graphene_vec3_t *color);

gboolean
openvr_overlay_set_color (OpenVROverlay *self, const graphene_vec3_t *color);

gboolean
openvr_overlay_set_alpha (OpenVROverlay *self, float alpha);

gboolean
openvr_overlay_set_width_meters (OpenVROverlay *self, float meters);

gboolean
openvr_overlay_set_transform_absolute (OpenVROverlay *self,
                                       graphene_matrix_t *mat);

gboolean
openvr_overlay_intersects (OpenVROverlay      *overlay,
                           graphene_point3d_t *intersection_point,
                           graphene_matrix_t  *transform);

// extends the overlay to an infinite plane, then calculates the intersection
// useful when implementing grab&move of overlays along their current axes
// via pointing a controller to the desired destination
gboolean
openvr_overlay_intersects_ray (OpenVROverlay      *overlay,
                               graphene_ray_t     *ray,
                               graphene_point3d_t *intersection_point);

bool
openvr_overlay_set_raw (OpenVROverlay *self, guchar *pixels,
                        uint32_t width, uint32_t height, uint32_t depth);

bool
openvr_overlay_set_gdk_pixbuf_raw (OpenVROverlay *self, GdkPixbuf * pixbuf);

G_END_DECLS

#endif /* OPENVR_GLIB_OVERLAY_H_ */
