/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_BUTTON_H_
#define OPENVR_GLIB_BUTTON_H_

#include <glib-object.h>
#include "openvr-overlay.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_BUTTON openvr_button_get_type()
G_DECLARE_FINAL_TYPE (OpenVRButton, openvr_button, OPENVR, BUTTON,
                      OpenVROverlay)

struct _OpenVRButton
{
  OpenVROverlay parent;
};

OpenVRButton *
openvr_button_new (gchar *id, gchar *text);

G_END_DECLS

#endif /* OPENVR_GLIB_BUTTON_H_ */
