/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib/gprintf.h>
#include <openvr_capi.h>
#include "openvr_capi_global.h"

#include "openvr-compositor.h"

#include "openvr-global.h"

G_DEFINE_TYPE (OpenVRCompositor, openvr_compositor, G_TYPE_OBJECT)

static void
openvr_compositor_class_init (OpenVRCompositorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_compositor_finalize;
}

static void
openvr_compositor_init (OpenVRCompositor *self)
{
  self->functions = NULL;
}

gboolean
_compositor_init_fn_table (OpenVRCompositor *self)
{
  INIT_FN_TABLE (self->functions, Compositor);
}

OpenVRCompositor *
openvr_compositor_new (void)
{
  return (OpenVRCompositor*) g_object_new (OPENVR_TYPE_COMPOSITOR, 0);
}

static void
openvr_compositor_finalize (GObject *gobject)
{
  // OpenVRCompositor *self = OPENVR_COMPOSITOR (gobject);
}
