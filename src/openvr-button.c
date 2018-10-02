/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-button.h"

G_DEFINE_TYPE (OpenVRButton, openvr_button, G_TYPE_OBJECT)

static void
openvr_button_finalize (GObject *gobject);

static void
openvr_button_class_init (OpenVRButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_button_finalize;
}

static void
openvr_button_init (OpenVRButton *self)
{
  self->index = 1337;
}

OpenVRButton *
openvr_button_new (void)
{
  return (OpenVRButton*) g_object_new (OPENVR_TYPE_BUTTON, 0);
}

static void
openvr_button_finalize (GObject *gobject)
{
  OpenVRButton *self = OPENVR_BUTTON (gobject);
  (void) self;
}
