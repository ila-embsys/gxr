/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_DEVICE_H_
#define GXR_DEVICE_H_

#if !defined(GXR_INSIDE) && !defined(GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>

#include <graphene.h>

#include <gulkan.h>

G_BEGIN_DECLS

#define GXR_TYPE_DEVICE gxr_device_get_type ()
G_DECLARE_DERIVABLE_TYPE (GxrDevice, gxr_device, GXR, DEVICE, GObject)

/**
 * GxrDeviceClass:
 * @parent: The parent class
 */
struct _GxrDeviceClass
{
  GObjectClass parent;
};

GxrDevice *
gxr_device_new (guint64 device_id);

gboolean
gxr_device_initialize (GxrDevice *self);

gboolean
gxr_device_is_controller (GxrDevice *self);

void
gxr_device_set_is_pose_valid (GxrDevice *self, bool valid);

gboolean
gxr_device_is_pose_valid (GxrDevice *self);

void
gxr_device_set_transformation_direct (GxrDevice *self, graphene_matrix_t *mat);

void
gxr_device_get_transformation_direct (GxrDevice *self, graphene_matrix_t *mat);

guint64
gxr_device_get_handle (GxrDevice *self);

void
gxr_device_set_handle (GxrDevice *self, guint64 handle);

G_END_DECLS

#endif /* GXR_DEVICE_H_ */
