/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_BUTTON_H_
#define OPENVR_GLIB_BUTTON_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_BUTTON openvr_button_get_type()
G_DECLARE_FINAL_TYPE (OpenVRButton, openvr_button, OPENVR, BUTTON, GObject)

struct _OpenVRButton
{
  GObject parent;

  guint index;
};

OpenVRButton *openvr_button_new (void);

G_END_DECLS

#endif /* OPENVR_GLIB_BUTTON_H_ */
