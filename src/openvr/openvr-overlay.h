/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OVERLAY_H_
#define GXR_OVERLAY_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <stdint.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>

#include "openvr-action.h"

#include <graphene.h>
#include <gulkan.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_OVERLAY openvr_overlay_get_type()
G_DECLARE_DERIVABLE_TYPE (OpenVROverlay, openvr_overlay, OPENVR, OVERLAY, GObject)

struct _OpenVROverlayClass
{
  GObjectClass parent_class;
};

/**
 * OpenVRPixelSize:
 * @width: Width
 * @height: Height
 *
 * Size in pixels.
 **/
typedef struct {
  uint32_t width;
  uint32_t height;
} OpenVRPixelSize;

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
openvr_overlay_set_raw (OpenVROverlay *self, guchar *pixels,
                        uint32_t width, uint32_t height, uint32_t depth);

gboolean
openvr_overlay_set_cairo_surface_raw (OpenVROverlay   *self,
                                      cairo_surface_t *surface);

gboolean
openvr_overlay_set_gdk_pixbuf_raw (OpenVROverlay *self, GdkPixbuf * pixbuf);

gboolean
openvr_overlay_get_size_pixels (OpenVROverlay *self, OpenVRPixelSize *size);

gboolean
openvr_overlay_get_width_meters (OpenVROverlay *self, float *width);

gboolean
openvr_overlay_get_size_meters (OpenVROverlay *self, graphene_vec2_t *size);

gboolean
openvr_overlay_enable_mouse_input (OpenVROverlay *self);

gboolean
openvr_overlay_get_transform_absolute (OpenVROverlay *self,
                                       graphene_matrix_t *mat);

gboolean
openvr_overlay_show_keyboard (OpenVROverlay *self);

void
openvr_overlay_set_keyboard_position (OpenVROverlay   *self,
                                      graphene_vec2_t *top_left,
                                      graphene_vec2_t *bottom_right);

gboolean
openvr_overlay_set_translation (OpenVROverlay      *self,
                                graphene_point3d_t *translation);

gboolean
openvr_overlay_destroy (OpenVROverlay *self);

gboolean
openvr_overlay_set_model (OpenVROverlay *self,
                          gchar *name,
                          graphene_vec4_t *color);

gboolean
openvr_overlay_get_model (OpenVROverlay *self, gchar *name,
                          graphene_vec4_t *color, uint32_t *id);

void
openvr_overlay_set_flip_y (OpenVROverlay *self,
                           gboolean flip_y);

bool
openvr_overlay_submit_texture (OpenVROverlay *self,
                               GulkanClient  *client,
                               GulkanTexture *texture);

gboolean
openvr_overlay_print_info (OpenVROverlay *self);

G_END_DECLS

#endif /* GXR_OVERLAY_H_ */