/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-system.h"
#include <openvr.h>

G_DEFINE_TYPE (OpenVRSystem, openvr_system, G_TYPE_OBJECT)

static void
openvr_system_class_init (OpenVRSystemClass *klass)
{
  g_print("init system class\n");
}

static void
openvr_system_init (OpenVRSystem *self)
{
  g_print("init system\n");
}

OpenVRSystem *
openvr_system_new (void)
{
  return static_cast<OpenVRSystem*>(g_object_new (OPENVR_TYPE_SYSTEM, 0));
}
