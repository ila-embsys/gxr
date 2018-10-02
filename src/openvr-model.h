/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_MODEL_H_
#define OPENVR_GLIB_MODEL_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_MODEL openvr_model_get_type()
G_DECLARE_FINAL_TYPE (OpenVRModel, openvr_model, OPENVR, MODEL, GObject)

struct _OpenVRModel
{
  GObject parent;

  guint index;
};

OpenVRModel *openvr_model_new (void);

G_END_DECLS

#endif /* OPENVR_GLIB_MODEL_H_ */
