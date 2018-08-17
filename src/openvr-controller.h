/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_CONTROLLER_H_
#define OPENVR_GLIB_CONTROLLER_H_

#include "openvr-math.h"
#include "openvr-overlay.h"
#include <gdk/gdk.h>
#include <glib-object.h>
#include <openvr_capi.h>

G_BEGIN_DECLS

typedef struct ControllerState
{
  TrackedDeviceIndex_t unControllerDeviceIndex;
  gboolean             _button1_pressed;  // button 1
  gboolean             _button2_pressed;  // button 2
  gboolean             initialized;
  gboolean             _grip_pressed;  // button 9
} ControllerState;

gboolean init_controller (ControllerState *state, int hand);

gboolean trigger_events (ControllerState *state, OpenVROverlay *overlay);

// extends the overlay to an infinite plane, then calculates the intersection
// useful when implementing grab&move of overlays along their current axes
// via pointing a controller to the desired destination
gboolean
openvr_overlay_plane_intersect (ControllerState    *state,
                                OpenVROverlay      *overlay,
                                graphene_point3d_t *intersection_point);

G_END_DECLS
#endif /* OPENVR_GLIB_CONTROLLER_H_ */
