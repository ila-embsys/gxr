/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_COMPOSITOR_H_
#define OPENVR_GLIB_COMPOSITOR_H_

#include <glib-object.h>
#include <openvr_capi.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_COMPOSITOR openvr_compositor_get_type()
G_DECLARE_FINAL_TYPE (OpenVRCompositor, openvr_compositor, OPENVR, COMPOSITOR, GObject)

struct _OpenVRCompositor
{
  GObjectClass parent_class;

  struct VR_IVRCompositor_FnTable *functions;
};

OpenVRCompositor *openvr_compositor_new (void);

static void
openvr_compositor_finalize (GObject *gobject);


G_END_DECLS

#endif /* OPENVR_GLIB_COMPOSITOR_H_ */
