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

/**
 * gxr_device_new:
 * @device_id: The ID of the device.
 * @returns: A newly allocated #GxrDevice.
 *
 * Creates a new device.
 */
GxrDevice *
gxr_device_new (guint64 device_id);

/**
 * gxr_device_initialize:
 * @self: A #GxrDevice.
 * @returns: `TRUE` if the device was initialized successfully, `FALSE` otherwise.
 *
 * Initializes the device.
 */
gboolean
gxr_device_initialize (GxrDevice *self);

/**
 * gxr_device_is_controller:
 * @self: A #GxrDevice.
 * @returns: `TRUE` if the device is a controller, `FALSE` otherwise.
 *
 * Checks if the device is a controller.
 */
gboolean
gxr_device_is_controller (GxrDevice *self);

/**
 * gxr_device_set_is_pose_valid:
 * @self: A #GxrDevice.
 * @valid: Whether the device's pose is valid.
 *
 * Sets whether the device's pose is valid.
 */
void
gxr_device_set_is_pose_valid (GxrDevice *self, bool valid);

/**
 * gxr_device_is_pose_valid:
 * @self: A #GxrDevice.
 * @returns: `TRUE` if the device's pose is valid, `FALSE` otherwise.
 *
 * Checks if the device's pose is valid.
 */
gboolean
gxr_device_is_pose_valid (GxrDevice *self);

/**
 * gxr_device_set_transformation_direct:
 * @self: A #GxrDevice.
 * @mat: The transformation matrix.
 *
 * Sets the device's transformation directly.
 */
void
gxr_device_set_transformation_direct (GxrDevice *self, graphene_matrix_t *mat);

/**
 * gxr_device_get_transformation_direct:
 * @self: A #GxrDevice.
 * @mat: (out): A pointer to store the transformation matrix.
 *
 * Gets the device's transformation directly.
 */
void
gxr_device_get_transformation_direct (GxrDevice *self, graphene_matrix_t *mat);

/**
 * gxr_device_get_handle:
 * @self: A #GxrDevice.
 * @returns: The device's handle.
 */
guint64
gxr_device_get_handle (GxrDevice *self);

/**
 * gxr_device_set_handle:
 * @self: A #GxrDevice.
 * @handle: The device's handle.
 *
 * Sets the device's handle.
 */
void
gxr_device_set_handle (GxrDevice *self, guint64 handle);

G_END_DECLS

#endif /* GXR_DEVICE_H_ */
