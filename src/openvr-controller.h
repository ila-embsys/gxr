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

typedef struct OpenVRController3DEvent {
  graphene_matrix_t transform;
  gboolean has_intersection;
  graphene_point3d_t intersection_point;
} OpenVRController3DEvent;

typedef enum OpenVRButton {
  OPENVR_BUTTON_TRIGGER,
  OPENVR_BUTTON_MENU,
  OPENVR_BUTTON_GRIP,
  OPENVR_BUTTON_AXIS0
} OpenVRButton;

G_BEGIN_DECLS

#define OPENVR_TYPE_CONTROLLER openvr_controller_get_type()
G_DECLARE_FINAL_TYPE (OpenVRController, openvr_controller,
                      OPENVR, CONTROLLER, GObject)

struct _OpenVRController
{
  GObject parent;
  TrackedDeviceIndex_t index;
  gboolean             initialized;

  uint64_t             last_pressed_state;
};

OpenVRController *openvr_controller_new (void);

OpenVRController *openvr_controller_new_from_id (uint32_t id);

GSList *
openvr_controller_enumerate ();

gboolean
openvr_controller_poll_overlay_event (OpenVRController *self,
                                      OpenVROverlay    *overlay);

gboolean
openvr_controller_get_transformation (OpenVRController  *self,
                                      graphene_matrix_t *transform);

G_END_DECLS
#endif /* OPENVR_GLIB_CONTROLLER_H_ */
