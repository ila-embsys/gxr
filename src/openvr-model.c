/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-model.h"

G_DEFINE_TYPE (OpenVRModel, openvr_model, G_TYPE_OBJECT)

static void
openvr_model_finalize (GObject *gobject);

static void
openvr_model_class_init (OpenVRModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_model_finalize;
}

static void
openvr_model_init (OpenVRModel *self)
{
  self->index = 1337;
}

OpenVRModel *
openvr_model_new (void)
{
  return (OpenVRModel*) g_object_new (OPENVR_TYPE_MODEL, 0);
}

static void
openvr_model_finalize (GObject *gobject)
{
  OpenVRModel *self = OPENVR_MODEL (gobject);
  (void) self;
}
