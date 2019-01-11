/*
 * Xrd GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_GLIB_SCENE_WINDOW_H_
#define XRD_GLIB_SCENE_WINDOW_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_WINDOW xrd_scene_window_get_type()
G_DECLARE_FINAL_TYPE (XrdSceneWindow, xrd_scene_window, XRD, SCENE_WINDOW, GObject)

struct _XrdSceneWindow
{
  GObject parent;

  guint index;
};

XrdSceneWindow *xrd_scene_window_new (void);

G_END_DECLS

#endif /* XRD_GLIB_SCENE_WINDOW_H_ */
