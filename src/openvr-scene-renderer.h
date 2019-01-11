/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_SCENE_RENDERER_H_
#define OPENVR_GLIB_SCENE_RENDERER_H_

#define MAX_TRACKED_DEVICES 64

#include "openvr-context.h"
#include "openvr-vulkan-model.h"
#include <glib-object.h>

#include <gulkan-client.h>
#include <gulkan-device.h>
#include <gulkan-instance.h>
#include <gulkan-texture.h>
#include "gulkan-frame-buffer.h"
#include "gulkan-vertex-buffer.h"
#include "gulkan-uniform-buffer.h"
#include "openvr-vulkan-model-manager.h"
#include "xrd-scene-window.h"
#include "xrd-scene-pointer.h"

// Pipeline state objects
enum PipelineType
{
  PIPELINE_WINDOWS = 0,
  PIPELINE_POINTER,
  PIPELINE_DEVICE_MODELS,
  PIPELINE_COUNT
};

// Indices of descriptor sets for rendering
enum DescriptorSetType
{
  DESCRIPTOR_SET_LEFT_EYE_DEVICE_MODEL0 = 0,
  DESCRIPTOR_SET_LEFT_EYE_DEVICE_MODEL_MAX = DESCRIPTOR_SET_LEFT_EYE_DEVICE_MODEL0 + MAX_TRACKED_DEVICES,
  DESCRIPTOR_SET_RIGHT_EYE_DEVICE_MODEL0,
  DESCRIPTOR_SET_RIGHT_EYE_DEVICE_MODEL_MAX = DESCRIPTOR_SET_RIGHT_EYE_DEVICE_MODEL0 + MAX_TRACKED_DEVICES,
  DESCRIPTOR_SET_COUNT
};

typedef struct VertexDataScene
{
  graphene_point3d_t position;
  graphene_point_t   uv;
} VertexDataScene;

G_BEGIN_DECLS

#define OPENVR_TYPE_SCENE_RENDERER openvr_scene_renderer_get_type ()
G_DECLARE_FINAL_TYPE (OpenVRSceneRenderer, openvr_scene_renderer,
                      OPENVR, SCENE_RENDERER, GulkanClient)

struct _OpenVRSceneRenderer
{
  GulkanClient parent;

  VkSampleCountFlagBits msaa_sample_count;
  float super_sample_scale;

  TrackedDevicePose_t device_poses[MAX_TRACKED_DEVICES];
  graphene_matrix_t device_mats[MAX_TRACKED_DEVICES];

  OpenVRVulkanModelManager *model_manager;

  float near_clip;
  float far_clip;

  VkDescriptorPool descriptor_pool;
  VkDescriptorSet descriptor_sets[DESCRIPTOR_SET_COUNT];

  XrdSceneWindow *scene_window;

  VkShaderModule shader_modules[PIPELINE_COUNT * 2];
  VkPipeline pipelines[PIPELINE_COUNT];
  VkDescriptorSetLayout descriptor_set_layout;
  VkPipelineLayout pipeline_layout;
  VkPipelineCache pipeline_cache;

  XrdScenePointer *scene_pointer;

  graphene_matrix_t mat_head_pose;
  graphene_matrix_t mat_eye_pos[2];
  graphene_matrix_t mat_projection[2];

  GulkanFrameBuffer *framebuffer[2];

  uint32_t render_width;
  uint32_t render_height;

};

OpenVRSceneRenderer *openvr_scene_renderer_new (void);

bool openvr_scene_renderer_initialize (OpenVRSceneRenderer *self);

void openvr_scene_renderer_render (OpenVRSceneRenderer *self);

G_END_DECLS

#endif /* OPENVR_GLIB_SCENE_RENDERER_H_ */
