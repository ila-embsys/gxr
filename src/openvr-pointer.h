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

  float default_length;
  float length;
};

OpenVRPointer *openvr_pointer_new (void);

void
openvr_pointer_move (OpenVRPointer     *self,
                     graphene_matrix_t *transform);

void
openvr_pointer_set_length (OpenVRPointer *self,
                           float          length);

void
openvr_pointer_reset_length (OpenVRPointer *self);



G_END_DECLS

#endif /* OPENVR_GLIB_POINTER_H_ */
