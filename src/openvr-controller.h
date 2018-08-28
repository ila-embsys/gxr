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

typedef struct OpenVRController
{
  TrackedDeviceIndex_t index;
  gboolean             _button1_pressed;  // button 1
  gboolean             _button2_pressed;  // button 2
  gboolean             initialized;
  gboolean             _grip_pressed;  // button 9
} OpenVRController;

gboolean
openvr_controller_init (OpenVRController *self, int hand);

gboolean
openvr_controller_trigger_events (OpenVRController *self,
                                  OpenVROverlay    *overlay);

G_END_DECLS
#endif /* OPENVR_GLIB_CONTROLLER_H_ */
