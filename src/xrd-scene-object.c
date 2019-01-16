/*
 * Xrd GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-object.h"

G_DEFINE_TYPE (XrdSceneObject, xrd_scene_object, G_TYPE_OBJECT)

static void
xrd_scene_object_finalize (GObject *gobject);

static void
xrd_scene_object_class_init (XrdSceneObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_object_finalize;
}

static void
xrd_scene_object_init (XrdSceneObject *self)
{
  self->index = 1337;
}

XrdSceneObject *
xrd_scene_object_new (void)
{
  return (XrdSceneObject*) g_object_new (XRD_TYPE_SCENE_OBJECT, 0);
}

static void
xrd_scene_object_finalize (GObject *gobject)
{
  XrdSceneObject *self = XRD_SCENE_OBJECT (gobject);
  (void) self;
}
