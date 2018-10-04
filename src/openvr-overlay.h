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

struct _OpenVROverlay
{
  GObjectClass parent_class;

  VROverlayHandle_t overlay_handle;
  VROverlayHandle_t thumbnail_handle;
};

typedef struct PixelSize
{
  uint32_t width;
  uint32_t height;
} PixelSize;

typedef struct OpenVRHoverEvent
{
  graphene_point3d_t point;
  graphene_matrix_t  pose;
  float              distance;
} OpenVRHoverEvent;


OpenVROverlay *openvr_overlay_new (void);

gboolean
openvr_overlay_create (OpenVROverlay *self,
                       gchar* key,
                       gchar* name);

gboolean
openvr_overlay_create_width (OpenVROverlay *self,
                             gchar* key,
                             gchar* name,
                             float meters);

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
openvr_overlay_set_sort_order (OpenVROverlay *self, uint32_t sort_order);

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

gboolean
openvr_overlay_set_raw (OpenVROverlay *self, guchar *pixels,
                        uint32_t width, uint32_t height, uint32_t depth);

gboolean
openvr_overlay_set_cairo_surface_raw (OpenVROverlay   *self,
                                      cairo_surface_t *surface);

gboolean
openvr_overlay_set_gdk_pixbuf_raw (OpenVROverlay *self, GdkPixbuf * pixbuf);

gboolean
openvr_overlay_get_size_pixels (OpenVROverlay *self, PixelSize *size);

gboolean
openvr_overlay_get_width_meters (OpenVROverlay *self, float *width);

gboolean
openvr_overlay_get_size_meters (OpenVROverlay *self, graphene_vec2_t *size);

gboolean
openvr_overlay_get_2d_intersection (OpenVROverlay      *overlay,
                                    graphene_point3d_t *intersection_point,
                                    PixelSize          *size_pixels,
                                    graphene_point_t   *position_2d);
gboolean
openvr_overlay_enable_mouse_input (OpenVROverlay *self);

gboolean
openvr_overlay_get_transform_absolute (OpenVROverlay *self,
                                       graphene_matrix_t *mat);

gboolean
openvr_overlay_show_overlay_keyboard (OpenVROverlay *self);

void
openvr_overlay_set_overlay_keyboard_position (OpenVROverlay *self,
                                              graphene_vec2_t *top_left,
                                              graphene_vec2_t *bottom_right);

gboolean
openvr_overlay_set_translation (OpenVROverlay      *self,
                                graphene_point3d_t *translation);

void
openvr_overlay_emit_grab (OpenVROverlay *self);

void
openvr_overlay_emit_release (OpenVROverlay *self);

void
openvr_overlay_emit_hover_end (OpenVROverlay *self);

void
openvr_overlay_emit_hover (OpenVROverlay    *self,
                           OpenVRHoverEvent *event);

gboolean
openvr_overlay_destroy (OpenVROverlay *self);

G_END_DECLS

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

#endif /* OPENVR_GLIB_OVERLAY_H_ */
