/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_CONTEXT_H_
#define OPENVR_GLIB_CONTEXT_H_

#include <glib-object.h>
#include <openvr_capi.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_CONTEXT openvr_context_get_type()
G_DECLARE_FINAL_TYPE (OpenVRContext, openvr_context, OPENVR, CONTEXT, GObject)

struct _OpenVRContext
{
  GObject parent;

  struct VR_IVRSystem_FnTable *system;
};

OpenVRContext *openvr_context_new (void);

OpenVRContext *openvr_context_get_instance (void);

static void
openvr_context_finalize (GObject *gobject);


G_END_DECLS

#endif /* OPENVR_GLIB_CONTEXT_H_ */
