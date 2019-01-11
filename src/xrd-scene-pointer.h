/*
 * Xrd GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_GLIB_SCENE_POINTER_H_
#define XRD_GLIB_SCENE_POINTER_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_POINTER xrd_scene_pointer_get_type()
G_DECLARE_FINAL_TYPE (XrdScenePointer, xrd_scene_pointer, XRD, SCENE_POINTER, GObject)

struct _XrdScenePointer
{
  GObject parent;

  guint index;
};

XrdScenePointer *xrd_scene_pointer_new (void);

G_END_DECLS

#endif /* XRD_GLIB_SCENE_POINTER_H_ */
