/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-system.h"

/* in myobject.c */
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

GObject *
my_object_new (void)
{
     return g_object_new (OPENVR_TYPE_SYSTEM, 0);
}
