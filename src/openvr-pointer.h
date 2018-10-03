/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_POINTER_H_
#define OPENVR_GLIB_POINTER_H_

#include <glib-object.h>
#include "openvr-model.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_POINTER openvr_pointer_get_type()
G_DECLARE_FINAL_TYPE (OpenVRPointer, openvr_pointer, OPENVR, POINTER,
                      OpenVRModel)

struct _OpenVRPointer
{
  OpenVRModel parent;
};

OpenVRPointer *openvr_pointer_new (void);

G_END_DECLS

#endif /* OPENVR_GLIB_POINTER_H_ */
