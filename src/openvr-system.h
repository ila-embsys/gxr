/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_SYSTEM_H_
#define OPENVR_GLIB_SYSTEM_H_

#include <glib-2.0/glib-object.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_SYSTEM openvr_system_get_type()
G_DECLARE_FINAL_TYPE (OpenVRSystem, openvr_system, OPENVR, SYSTEM, GObject)

struct _OpenVRSystem
{
  GObjectClass parent_class;

  gboolean test;
};

G_DEFINE_TYPE (OpenVRSystem, openvr_system, G_TYPE_OBJECT)

OpenVRSystem *openvr_system_new (void);

G_END_DECLS

#endif /* OPENVR_GLIB_SYSTEM_H_ */
