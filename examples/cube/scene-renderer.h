/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef SCENE_RENDERER_H_
#define SCENE_RENDERER_H_

#include <glib-object.h>

#include <gxr.h>

G_BEGIN_DECLS

enum PipelineType
{
  PIPELINE_WINDOWS = 0,
  PIPELINE_TIP,
  PIPELINE_POINTER,
  PIPELINE_SELECTION,
  PIPELINE_BACKGROUND,
  PIPELINE_DEVICE_MODELS,
  PIPELINE_COUNT
};

#define SCENE_TYPE_RENDERER scene_renderer_get_type()
G_DECLARE_FINAL_TYPE (SceneRenderer, scene_renderer,
                      SCENE, RENDERER, GulkanRenderer)

SceneRenderer *
scene_renderer_new (void);

gboolean
scene_renderer_init_vulkan (SceneRenderer *self,
                            GxrContext    *context);

VkDescriptorSetLayout *
scene_renderer_get_descriptor_set_layout (SceneRenderer *self);

GulkanRenderPass *
scene_renderer_get_render_pass (SceneRenderer *self);

gboolean
scene_renderer_draw (SceneRenderer *self);

void
scene_renderer_set_render_cb (SceneRenderer *self,
                              void (*render_eye) (uint32_t         eye,
                                                  VkCommandBuffer  cmd_buffer,
                                                  VkPipelineLayout pipeline_layout,
                                                  VkPipeline      *pipelines,
                                                  gpointer         data),
                              gpointer scene_client);

void
scene_renderer_set_update_lights_cb (SceneRenderer *self,
                                     void (*update_lights) (gpointer data),
                                     gpointer scene_client);

VkBuffer
scene_renderer_get_lights_buffer_handle (SceneRenderer *self);

void
scene_renderer_update_lights (SceneRenderer *self,
                              GSList        *controllers);

GulkanClient *
scene_renderer_get_gulkan (SceneRenderer *self);

G_END_DECLS

#endif /* GXR_CUBE_RENDERER_H_ */