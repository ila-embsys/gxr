/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_SYSTEM_H_
#define OPENVR_GLIB_SYSTEM_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_SYSTEM openvr_system_get_type()
G_DECLARE_FINAL_TYPE (OpenVRSystem, openvr_system, OPENVR, SYSTEM, GObject)

struct _OpenVRSystem
{
  GObjectClass parent_class;

  gboolean test;
};

OpenVRSystem *openvr_system_new (void);

gboolean openvr_system_init_overlay (OpenVRSystem * self);


#ifdef __cplusplus

#endif


G_END_DECLS

#endif /* OPENVR_GLIB_SYSTEM_H_ */
