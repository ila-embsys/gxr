/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-pointer.h"

G_DEFINE_TYPE (OpenVRPointer, openvr_pointer, G_TYPE_OBJECT)

static void
openvr_pointer_finalize (GObject *gobject);

static void
openvr_pointer_class_init (OpenVRPointerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_pointer_finalize;
}

static void
openvr_pointer_init (OpenVRPointer *self)
{
  self->index = 1337;
}

OpenVRPointer *
openvr_pointer_new (void)
{
  return (OpenVRPointer*) g_object_new (OPENVR_TYPE_POINTER, 0);
}

static void
openvr_pointer_finalize (GObject *gobject)
{
  OpenVRPointer *self = OPENVR_POINTER (gobject);
  (void) self;
}
