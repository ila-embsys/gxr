/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_SCENE_DEVICE_MANAGER_H_
#define XRD_SCENE_DEVICE_MANAGER_H_

#include <glib-object.h>

#include "xrd-scene-device.h"
#include <gulkan-client.h>

#define MAX_TRACKED_DEVICES 64

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_DEVICE_MANAGER xrd_scene_device_manager_get_type()
G_DECLARE_FINAL_TYPE (XrdSceneDeviceManager, xrd_scene_device_manager,
                      XRD, SCENE_DEVICE_MANAGER, GObject)

struct _XrdSceneDeviceManager
{
  GObject parent;

  GHashTable *model_content;

  XrdSceneDevice *models[MAX_TRACKED_DEVICES];
  TrackedDevicePose_t device_poses[MAX_TRACKED_DEVICES];
  graphene_matrix_t device_mats[MAX_TRACKED_DEVICES];
};

XrdSceneDeviceManager *xrd_scene_device_manager_new (void);

void
xrd_scene_device_manager_load (XrdSceneDeviceManager *self,
                               GulkanClient          *client,
                               TrackedDeviceIndex_t   device_id,
                               VkDescriptorSetLayout *layout);

void
xrd_scene_device_manager_render (XrdSceneDeviceManager *self,
                                 EVREye                 eye,
                                 VkCommandBuffer        cmd_buffer,
                                 VkPipeline             pipeline,
                                 VkPipelineLayout       layout,
                                 graphene_matrix_t     *vp);

void
xrd_scene_device_manager_update_poses (XrdSceneDeviceManager *self,
                                       graphene_matrix_t     *mat_head_pose);

G_END_DECLS

#endif /* XRD_SCENE_DEVICE_MANAGER_H_ */
