/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_OVERLAY_MANAGER_H_
#define OPENVR_GLIB_OVERLAY_MANAGER_H_

#include <glib-object.h>

#include "openvr-overlay.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_OVERLAY_MANAGER openvr_overlay_manager_get_type()
G_DECLARE_FINAL_TYPE (OpenVROverlayManager, openvr_overlay_manager, OPENVR,
                      OVERLAY_MANAGER, GObject)

typedef struct OpenVRHoverEvent
{
  OpenVROverlay     *overlay;
  graphene_point3d_t point;
  graphene_matrix_t  pose;
  float              distance;
  OpenVROverlay     *previous_overlay;
} OpenVRHoverEvent;

typedef struct OpenVRNoHoverEvent
{
  OpenVROverlay     *previous_overlay;
  graphene_matrix_t  pose;
} OpenVRNoHoverEvent;

typedef struct TransformTransition {
  OpenVROverlay *overlay;
  graphene_matrix_t from;
  graphene_matrix_t to;
  float interpolate;
} TransformTransition;

typedef struct HoverState {
  OpenVROverlay    *overlay;
  graphene_matrix_t pose;
  float             distance;
} HoverState;

typedef struct GrabState {
  OpenVROverlay    *overlay;
  graphene_matrix_t pose;
} GrabState;

typedef enum
{
  OPENVR_OVERLAY_HOVER               = 1 << 0,
  OPENVR_OVERLAY_GRAB                = 1 << 1,
  OPENVR_OVERLAY_DESTROY_WITH_PARENT = 1 << 2
} OpenVROverlayFlags;

struct _OpenVROverlayManager
{
  GObject parent;

  GSList *grab_overlays;
  GSList *hover_overlays;
  GSList *destroy_overlays;

  HoverState hover_state;
  GrabState grab_state;

  GHashTable *reset_transforms;
};

OpenVROverlayManager *openvr_overlay_manager_new (void);

void
openvr_overlay_manager_arrange_reset (OpenVROverlayManager *self);

gboolean
openvr_overlay_manager_arrange_sphere (OpenVROverlayManager *self,
                                       uint32_t              grid_width,
                                       uint32_t              grid_height);

void
openvr_overlay_manager_add_overlay (OpenVROverlayManager *self,
                                    OpenVROverlay        *overlay,
                                    OpenVROverlayFlags    flags);

void
openvr_overlay_manager_test_hover (OpenVROverlayManager *self,
                                   graphene_matrix_t    *pose);

float
openvr_overlay_manager_get_hover_distance (OpenVROverlayManager *self);

void
openvr_overlay_manager_drag_overlay (OpenVROverlayManager *self,
                                     graphene_matrix_t    *pose);

gboolean
openvr_overlay_manager_is_grabbing (OpenVROverlayManager *self);

gboolean
openvr_overlay_manager_is_hovering (OpenVROverlayManager *self);

gboolean
openvr_overlay_manager_is_hovered (OpenVROverlayManager *self,
                                   OpenVROverlay        *overlay);

void
openvr_overlay_manager_drag_start (OpenVROverlayManager *self);

OpenVROverlay*
openvr_overlay_manager_grab_end (OpenVROverlayManager *self);

G_END_DECLS

#endif /* OPENVR_GLIB_OVERLAY_MANAGER_H_ */
