/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
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

#define OPENVR_TYPE_CONTROLLER openvr_controller_get_type()
G_DECLARE_FINAL_TYPE (OpenVRController, openvr_controller, OPENVR, CONTROLLER, GObject)

struct _OpenVRController
{
  GObject parent;
  TrackedDeviceIndex_t index;
  gboolean             _button1_pressed;  // button 1
  gboolean             _button2_pressed;  // button 2
  gboolean             initialized;
  gboolean             _grip_pressed;  // button 9
};

OpenVRController *openvr_controller_new (void);

gboolean
openvr_controller_find_by_id (OpenVRController *self, int id);

gboolean
openvr_controller_trigger_events (OpenVRController *self,
                                  OpenVROverlay    *overlay);

G_END_DECLS
#endif /* OPENVR_GLIB_CONTROLLER_H_ */
