/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-overlay.h"
#include <openvr.h>

G_DEFINE_TYPE (OpenVROverlay, openvr_overlay, G_TYPE_OBJECT)

static void
openvr_overlay_class_init (OpenVROverlayClass *klass)
{}

static void
openvr_overlay_init (OpenVROverlay *self)
{}

OpenVROverlay *
openvr_overlay_new (void)
{
  return static_cast<OpenVROverlay*> (g_object_new (OPENVR_TYPE_OVERLAY, 0));
}
