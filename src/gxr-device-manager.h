/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXRDEVICE_MANAGER_H_
#define GXRDEVICE_MANAGER_H_

#include <glib-object.h>
#include <gulkan.h>

#include "gxr-action-set.h"
#include "gxr-device.h"

G_BEGIN_DECLS

#define GXR_TYPE_DEVICE_MANAGER gxr_device_manager_get_type ()
G_DECLARE_FINAL_TYPE (GxrDeviceManager,
                      gxr_device_manager,
                      GXR,
                      DEVICE_MANAGER,
                      GObject)

#ifndef __GTK_DOC_IGNORE__
typedef struct _GxrContext GxrContext;
#endif

/**
 * GxrPose:
 * @transformation: The #graphene_matrix_t.
 * @is_valid: Validity of the pose.
 *
 * A 4x4 matrix pose.
 **/
// clang-format off
// https://gitlab.gnome.org/GNOME/gtk-doc/-/issues/91
typedef struct {
  graphene_matrix_t transformation;
  gboolean          is_valid;
} GxrPose;
// clang-format on

/**
 * gxr_device_manager_new:
 * @returns: A newly allocated #GxrDeviceManager.
 *
 * Creates a new device manager.
 */
GxrDeviceManager *
gxr_device_manager_new (void);

/**
 * gxr_device_manager_add:
 * @self: A #GxrDeviceManager.
 * @device_id: The ID of the device.
 * @is_controller: Whether the device is a controller.
 * @returns: `TRUE` if the device was added successfully, `FALSE` otherwise.
 *
 * Adds a new device to the device manager.
 *
 */
gboolean
gxr_device_manager_add (GxrDeviceManager *self,
                        guint64           device_id,
                        bool              is_controller);

/**
 * gxr_device_manager_remove:
 * @self: A #GxrDeviceManager.
 * @device_id: The ID of the device.
 *
 * Removes a device from the device manager.
 */
void
gxr_device_manager_remove (GxrDeviceManager *self, guint64 device_id);

/**
 * gxr_device_manager_update_poses:
 * @self: A #GxrDeviceManager.
 * @poses: A #GList of #GxrPose objects.
 *
 * Updates the poses of the devices in the device manager.
 */
void
gxr_device_manager_update_poses (GxrDeviceManager *self, GxrPose *poses);

/**
 * gxr_device_manager_get_controllers:
 * @self: A #GxrDeviceManager.
 * @returns: (element-type GxrController) (transfer container): A newly allocated #GSList containing #GxrController objects that are controllers.
 *
 * Retrieves a list of controllers from the device manager.
 */
GSList *
gxr_device_manager_get_controllers (GxrDeviceManager *self);

/**
 * gxr_device_manager_get:
 * @self: A #GxrDeviceManager.
 * @device_id: The ID of the device.
 * @returns: (transfer none): A #GxrDevice object, or NULL if no device with the given ID exists.
 *
 * Retrieves a device from the device manager by its ID.
 */
GxrDevice *
gxr_device_manager_get (GxrDeviceManager *self, guint64 device_id);

/**
 * gxr_device_manager_get_devices:
 * @self: A #GxrDeviceManager.
 * @returns: (element-type GxrDevice) (transfer container): A newly allocated
 * #GList containing #GxrDevice objects.
 * 
 * Retrieves a list of devices managed by the device manager.
 */
GList *
gxr_device_manager_get_devices (GxrDeviceManager *self);

/**
 * gxr_device_manager_connect_pose_actions:
 * @self: A #GxrDeviceManager.
 * @action_set: The action set to connect the poses to.
 * @pointer_pose_url: The URL of the pointer pose.
 * @hand_grip_pose_url: The URL of the hand grip pose.
 *
 * Connects the pointer and hand grip poses to the specified action set.
 */
void
gxr_device_manager_connect_pose_actions (GxrDeviceManager *self,
                                         GxrActionSet     *action_set,
                                         gchar            *pointer_pose_url,
                                         gchar            *hand_grip_pose_url);

G_END_DECLS

#endif /* GXRDEVICE_MANAGER_H_ */
