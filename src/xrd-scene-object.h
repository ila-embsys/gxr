/*
 * Xrd GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_GLIB_SCENE_OBJECT_H_
#define XRD_GLIB_SCENE_OBJECT_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_OBJECT xrd_scene_object_get_type()
G_DECLARE_FINAL_TYPE (XrdSceneObject, xrd_scene_object, XRD, SCENE_OBJECT, GObject)

struct _XrdSceneObject
{
  GObject parent;

  guint index;
};

XrdSceneObject *xrd_scene_object_new (void);

G_END_DECLS

#endif /* XRD_GLIB_SCENE_OBJECT_H_ */
