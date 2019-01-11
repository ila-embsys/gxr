/*
 * Xrd GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-pointer.h"

G_DEFINE_TYPE (XrdScenePointer, xrd_scene_pointer, G_TYPE_OBJECT)

static void
xrd_scene_pointer_finalize (GObject *gobject);

static void
xrd_scene_pointer_class_init (XrdScenePointerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_pointer_finalize;
}

static void
xrd_scene_pointer_init (XrdScenePointer *self)
{
  self->index = 1337;
}

XrdScenePointer *
xrd_scene_pointer_new (void)
{
  return (XrdScenePointer*) g_object_new (XRD_TYPE_SCENE_POINTER, 0);
}

static void
xrd_scene_pointer_finalize (GObject *gobject)
{
  XrdScenePointer *self = XRD_SCENE_POINTER (gobject);
  (void) self;
}
