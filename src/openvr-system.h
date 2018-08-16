/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_SYSTEM_H_
#define OPENVR_GLIB_SYSTEM_H_

#include <glib-object.h>
#include <openvr_capi.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_SYSTEM openvr_system_get_type()
G_DECLARE_FINAL_TYPE (OpenVRSystem, openvr_system, OPENVR, SYSTEM, GObject)

struct _OpenVRSystem
{
  GObjectClass parent_class;
};

OpenVRSystem *openvr_system_new (void);

void openvr_system_print_device_info (OpenVRSystem * self);

G_END_DECLS

#endif /* OPENVR_GLIB_SYSTEM_H_ */
