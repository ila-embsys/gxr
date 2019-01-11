/*
 * Xrd GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-window.h"

G_DEFINE_TYPE (XrdSceneWindow, xrd_scene_window, G_TYPE_OBJECT)

static void
xrd_scene_window_finalize (GObject *gobject);

static void
xrd_scene_window_class_init (XrdSceneWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_window_finalize;
}

static void
xrd_scene_window_init (XrdSceneWindow *self)
{
  self->index = 1337;
}

XrdSceneWindow *
xrd_scene_window_new (void)
{
  return (XrdSceneWindow*) g_object_new (XRD_TYPE_SCENE_WINDOW, 0);
}

static void
xrd_scene_window_finalize (GObject *gobject)
{
  XrdSceneWindow *self = XRD_SCENE_WINDOW (gobject);
  (void) self;
}
