/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include "openvr-scene-renderer.h"

#include <gmodule.h>
#include <gulkan-texture.h>
#include <gulkan-renderer.h>
#include "gulkan-geometry.h"

#include "openvr-compositor.h"
#include "openvr-system.h"
#include "openvr-math.h"

#include <signal.h>

static bool use_validation = true;

G_DEFINE_TYPE (OpenVRSceneRenderer, openvr_scene_renderer, GULKAN_TYPE_CLIENT)

static void openvr_scene_renderer_finalize (GObject *gobject);

bool _init_vulkan (OpenVRSceneRenderer *self);
bool _init_vulkan_instance (OpenVRSceneRenderer *self);
bool _init_vulkan_device (OpenVRSceneRenderer *self);
void _init_device_model (OpenVRSceneRenderer *self,
                         TrackedDeviceIndex_t device_id);
void _init_device_models (OpenVRSceneRenderer *self);
bool _init_stereo_render_targets (OpenVRSceneRenderer *self);
bool _init_shaders (OpenVRSceneRenderer *self);
void _init_descriptor_sets (OpenVRSceneRenderer *self);
bool _init_graphics_pipelines (OpenVRSceneRenderer *self);
bool _init_pipeline_cache (OpenVRSceneRenderer *self);
bool _init_pipeline_layout (OpenVRSceneRenderer *self);
bool _init_descriptor_layout (OpenVRSceneRenderer *self);
void _init_descriptor_pool (OpenVRSceneRenderer *self);

graphene_matrix_t _get_hmd_pose_matrix (EVREye eye);
graphene_matrix_t _get_view_projection_matrix (OpenVRSceneRenderer *self,
                                               EVREye               eye);

void _update_matrices (OpenVRSceneRenderer *self);
void _update_device_poses (OpenVRSceneRenderer *self);
void _render_stereo (OpenVRSceneRenderer *self, VkCommandBuffer cmd_buffer);

static void
openvr_scene_renderer_class_init (OpenVRSceneRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_scene_renderer_finalize;
}

static void
openvr_scene_renderer_init (OpenVRSceneRenderer *self)
{
  self->msaa_sample_count = VK_SAMPLE_COUNT_4_BIT;
  self->super_sample_scale = 1.0f;

  self->descriptor_pool = VK_NULL_HANDLE;

  self->descriptor_set_layout = VK_NULL_HANDLE;
  self->pipeline_layout = VK_NULL_HANDLE;
  self->pipeline_cache = VK_NULL_HANDLE;

  memset (&self->shader_modules[0], 0, sizeof (self->shader_modules));
  memset (&self->pipelines[0], 0, sizeof (self->pipelines));

  memset (self->descriptor_sets, 0, sizeof (self->descriptor_sets));

  self->scene_pointer = xrd_scene_pointer_new ();

  self->near_clip = 0.1f;
  self->far_clip = 30.0f;

  self->framebuffer[EVREye_Eye_Left] = gulkan_frame_buffer_new();
  self->framebuffer[EVREye_Eye_Right] = gulkan_frame_buffer_new();

  self->model_manager = openvr_vulkan_model_manager_new ();

  self->scene_window = xrd_scene_window_new ();
}

OpenVRSceneRenderer *
openvr_scene_renderer_new (void)
{
  return (OpenVRSceneRenderer *)g_object_new (OPENVR_TYPE_SCENE_RENDERER, 0);
}

static void
openvr_scene_renderer_finalize (GObject *gobject)
{
  OpenVRSceneRenderer *self = OPENVR_SCENE_RENDERER (gobject);

  GulkanClient *client = GULKAN_CLIENT (gobject);
  VkDevice device = client->device->device;

  if (device != VK_NULL_HANDLE)
    vkDeviceWaitIdle (device);

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  g_object_unref (self->model_manager);

  if (device != VK_NULL_HANDLE)
    {
      vkDestroyDescriptorPool (device, self->descriptor_pool, NULL);

      g_object_unref (self->scene_window);

      for (uint32_t eye = 0; eye < 2; eye++)
        {
          g_object_unref (self->framebuffer[eye]);
        }

      g_object_unref (self->scene_pointer);

      vkDestroyPipelineLayout (device, self->pipeline_layout, NULL);
      vkDestroyDescriptorSetLayout (device, self->descriptor_set_layout, NULL);
      for (uint32_t i = 0; i < PIPELINE_COUNT; i++)
        vkDestroyPipeline (device, self->pipelines[i], NULL);

      for (uint32_t i = 0; i < G_N_ELEMENTS (self->shader_modules); i++)
        vkDestroyShaderModule (device, self->shader_modules[i], NULL);

      vkDestroyPipelineCache (device, self->pipeline_cache, NULL);
    }

  G_OBJECT_CLASS (openvr_scene_renderer_parent_class)->finalize (gobject);
}

bool
_init_openvr ()
{
  if (!openvr_context_is_installed ())
    {
      g_printerr ("VR Runtime not installed.\n");
      return false;
    }

  OpenVRContext *context = openvr_context_get_instance ();
  if (!openvr_context_init_scene (context))
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  if (!openvr_context_is_valid (context))
    {
      g_printerr ("Could not load OpenVR function pointers.\n");
      return false;
    }

  return true;
}

GdkPixbuf *
load_gdk_pixbuf ()
{
  GError *error = NULL;
  GdkPixbuf *pixbuf_rgb = gdk_pixbuf_new_from_resource ("/res/cat.jpg", &error);

  if (error != NULL)
    {
      g_printerr ("Unable to read file: %s\n", error->message);
      g_error_free (error);
      return NULL;
    }
  else
    {
      GdkPixbuf *pixbuf_rgba = gdk_pixbuf_add_alpha (pixbuf_rgb, false, 0, 0, 0);
      g_object_unref (pixbuf_rgb);
      return pixbuf_rgba;
    }
}

static void
_device_activate_cb (OpenVRContext          *context,
                     OpenVRDeviceIndexEvent *event,
                     gpointer               _self)
{
  (void) context;
  OpenVRSceneRenderer *self = (OpenVRSceneRenderer*) _self;
  g_print ("Device %d activated, initializing model.\n", event->index);
  _init_device_model (self, event->index);
}

static void
_device_deactivate_cb (OpenVRContext          *context,
                       OpenVRDeviceIndexEvent *event,
                       gpointer               _self)
{
  (void) context;
  OpenVRSceneRenderer *self = (OpenVRSceneRenderer*) _self;
  g_print ("Device %d deactivated. Removing model node.\n", event->index);
  g_object_unref (self->model_manager->models[event->index]);
  self->model_manager->models[event->index] = NULL;
}

gboolean
_poll_events_cb (gpointer unused)
{
  (void) unused;
  OpenVRContext *context = openvr_context_get_instance ();
  openvr_context_poll_event (context);

  return TRUE;
}

bool
openvr_scene_renderer_initialize (OpenVRSceneRenderer *self)
{
  if (!_init_openvr ())
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  if (!_init_vulkan (self))
    {
      g_print ("Could not init Vulkan.\n");
      return false;
    }

  OpenVRContext *context = openvr_context_get_instance ();
  g_signal_connect (context, "device-activate-event",
                    (GCallback) _device_activate_cb, self);
  g_signal_connect (context, "device-deactivate-event",
                    (GCallback) _device_deactivate_cb, self);

  g_timeout_add (20, _poll_events_cb, &self);

  return true;
}

bool
_init_vulkan (OpenVRSceneRenderer *self)
{
  if (!_init_vulkan_instance (self))
    return false;

  if (!_init_vulkan_device (self))
    return false;

  GulkanClient *client = GULKAN_CLIENT (self);
  if (!gulkan_client_init_command_pool (client))
    {
      g_printerr ("Could not create command pool.\n");
      return false;
    }

  FencedCommandBuffer cmd_buffer;
  if (!gulkan_client_begin_res_cmd_buffer (client, &cmd_buffer))
    {
      g_printerr ("Could not begin command buffer.\n");
      return false;
    }

  GdkPixbuf *pixbuf = load_gdk_pixbuf ();
  if (!pixbuf)
    return false;

  if (!xrd_scene_window_init_texture (self->scene_window, client->device,
                                      cmd_buffer.cmd_buffer, pixbuf))
    return FALSE;

  xrd_scene_window_init_geometry (self->scene_window, client->device);

  _update_matrices (self);
  _init_stereo_render_targets (self);

  if (!_init_shaders (self))
    return false;

  if (!_init_descriptor_layout (self))
    return false;
  if (!_init_pipeline_layout (self))
    return false;
  if (!_init_pipeline_cache (self))
    return false;
  if (!_init_graphics_pipelines (self))
    return false;

  _init_descriptor_pool (self);
  _init_descriptor_sets (self);
  _init_device_models (self);

  if (!gulkan_client_submit_res_cmd_buffer (client, &cmd_buffer))
    {
      g_printerr ("Could not submit command buffer.\n");
      return false;
    }

  vkQueueWaitIdle (client->device->queue);

  return true;
}

bool
_init_vulkan_instance (OpenVRSceneRenderer *self)
{
  GSList *extensions = NULL;
  GulkanClient *client = GULKAN_CLIENT (self);
  openvr_compositor_get_instance_extensions (&extensions);
  return gulkan_instance_create (client->instance, use_validation,
                                 extensions);
}

bool
_init_vulkan_device (OpenVRSceneRenderer *self)
{
  /* Query OpenVR for a physical device */
  GulkanClient *client = GULKAN_CLIENT (self);
  uint64_t physical_device = 0;
  OpenVRContext *context = openvr_context_get_instance ();
  context->system->GetOutputDevice (
      &physical_device, ETextureType_TextureType_Vulkan,
      (struct VkInstance_T *) client->instance->instance);

  GSList *extensions = NULL;
  openvr_compositor_get_device_extensions ((VkPhysicalDevice)physical_device,
                                           &extensions);

  return gulkan_device_create (client->device, client->instance,
                               (VkPhysicalDevice)physical_device, extensions);
}

void
_init_device_model (OpenVRSceneRenderer *self,
                    TrackedDeviceIndex_t device_id)
{
  VkDescriptorSet descriptor_sets[2] = {
    self->descriptor_sets[DESCRIPTOR_SET_LEFT_EYE_DEVICE_MODEL0 + device_id],
    self->descriptor_sets[DESCRIPTOR_SET_RIGHT_EYE_DEVICE_MODEL0 + device_id],
  };

  GulkanClient *client = GULKAN_CLIENT (self);
  openvr_vulkan_model_manager_load (self->model_manager, client,
                                    descriptor_sets, device_id);
}

void
_init_device_models (OpenVRSceneRenderer *self)
{
  for (TrackedDeviceIndex_t i = k_unTrackedDeviceIndex_Hmd + 1;
       i < k_unMaxTrackedDeviceCount; i++)
    {
      OpenVRContext *context = openvr_context_get_instance ();
      if (!context->system->IsTrackedDeviceConnected (i))
        continue;

      _init_device_model (self, i);
    }
}

void
openvr_scene_renderer_render (OpenVRSceneRenderer *self)
{
  GulkanClient *client = GULKAN_CLIENT (self);

  FencedCommandBuffer cmd_buffer;
  gulkan_client_begin_res_cmd_buffer (client, &cmd_buffer);

  /* Don't update the pointer if there is no input */
  OpenVRContext *context = openvr_context_get_instance ();
  if (context->system->IsInputAvailable ())
    xrd_scene_pointer_update (self->scene_pointer, client->device,
                              self->device_poses, self->device_mats);

  _render_stereo (self, cmd_buffer.cmd_buffer);

  vkEndCommandBuffer (cmd_buffer.cmd_buffer);

  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &cmd_buffer.cmd_buffer,
    .waitSemaphoreCount = 0,
    .pWaitSemaphores = NULL,
    .signalSemaphoreCount = 0
  };

  vkQueueSubmit (client->device->queue, 1, &submit_info, cmd_buffer.fence);

  vkQueueWaitIdle (client->device->queue);

  vkFreeCommandBuffers (client->device->device, client->command_pool,
                        1, &cmd_buffer.cmd_buffer);
  vkDestroyFence (client->device->device, cmd_buffer.fence, NULL);

  GulkanDevice *device = client->device;

  VRVulkanTextureData_t openvr_texture_data = {
    .m_nImage = (uint64_t)self->framebuffer[EVREye_Eye_Left]->color_image,
    .m_pDevice = (struct VkDevice_T *) device->device,
    .m_pPhysicalDevice = (struct VkPhysicalDevice_T *) device->physical_device,
    .m_pInstance = (struct VkInstance_T *) client->instance->instance,
    .m_pQueue = (struct VkQueue_T *) device->queue,
    .m_nQueueFamilyIndex = device->queue_family_index,
    .m_nWidth = self->render_width,
    .m_nHeight = self->render_height,
    .m_nFormat = VK_FORMAT_R8G8B8A8_UNORM,
    .m_nSampleCount = self->msaa_sample_count
  };

  Texture_t texture = {
    &openvr_texture_data,
    ETextureType_TextureType_Vulkan,
    EColorSpace_ColorSpace_Auto
  };

  VRTextureBounds_t bounds = {
    .uMin = 0.0f,
    .uMax = 1.0f,
    .vMin = 0.0f,
    .vMax = 1.0f
  };

  context->compositor->Submit (EVREye_Eye_Left, &texture, &bounds,
                               EVRSubmitFlags_Submit_Default);

  openvr_texture_data.m_nImage =
    (uint64_t) self->framebuffer[EVREye_Eye_Right]->color_image;
  context->compositor->Submit (EVREye_Eye_Right, &texture, &bounds,
                               EVRSubmitFlags_Submit_Default);

  _update_device_poses (self);
}

bool
_init_stereo_render_targets (OpenVRSceneRenderer *self)
{
  OpenVRContext *context = openvr_context_get_instance ();
  GulkanClient *client = GULKAN_CLIENT (self);

  context->system->GetRecommendedRenderTargetSize (&self->render_width,
                                                   &self->render_height);
  self->render_width =
      (uint32_t) (self->super_sample_scale * (float) self->render_width);
  self->render_height =
      (uint32_t) (self->super_sample_scale * (float) self->render_height);

  for (uint32_t eye = 0; eye < 2; eye++)
    gulkan_frame_buffer_initialize (self->framebuffer[eye],
                                    client->device,
                                    self->render_width, self->render_height,
                                    self->msaa_sample_count,
                                    VK_FORMAT_R8G8B8A8_UNORM);
  return true;
}

void
_update_matrices (OpenVRSceneRenderer *self)
{
  for (uint32_t eye = 0; eye < 2; eye++)
    {
      self->mat_projection[eye] =
        openvr_system_get_projection_matrix (eye,
                                             self->near_clip,
                                             self->far_clip);
      self->mat_eye_pos[eye] = _get_hmd_pose_matrix (eye);
    }
}

void
_render_device_models (OpenVRSceneRenderer *self,
                       EVREye eye, VkCommandBuffer cmd_buffer)
{
  vkCmdBindPipeline (cmd_buffer,
                     VK_PIPELINE_BIND_POINT_GRAPHICS,
                     self->pipelines[PIPELINE_DEVICE_MODELS]);
  for (uint32_t i = 0; i < k_unMaxTrackedDeviceCount; i++)
    {
      if (!self->model_manager->models[i])
        continue;

      TrackedDevicePose_t *pose = &self->device_poses[i];
      if (!pose->bPoseIsValid)
        continue;

      OpenVRContext *context = openvr_context_get_instance ();
      if (!context->system->IsInputAvailable () &&
          context->system->GetTrackedDeviceClass (i) ==
              ETrackedDeviceClass_TrackedDeviceClass_Controller)
        continue;

      graphene_matrix_t mvp;
      graphene_matrix_init_from_matrix (&mvp, &self->device_mats[i]);

      graphene_matrix_t vp = _get_view_projection_matrix (self, eye);
      graphene_matrix_multiply (&mvp, &vp, &mvp);

      openvr_vulkan_model_draw (self->model_manager->models[i], eye,
                                cmd_buffer, self->pipeline_layout, &mvp);
    }
}

void
_render_stereo (OpenVRSceneRenderer *self, VkCommandBuffer cmd_buffer)
{
  VkViewport viewport = {
    0.0f, 0.0f,
    self->render_width, self->render_height,
    0.0f, 1.0f
  };
  vkCmdSetViewport (cmd_buffer, 0, 1, &viewport);
  VkRect2D scissor = {
    .offset = {0, 0},
    .extent = {self->render_width, self->render_height}
  };
  vkCmdSetScissor (cmd_buffer, 0, 1, &scissor);

  for (uint32_t eye = 0; eye < 2; eye++)
    {
      gulkan_frame_buffer_begin_pass (self->framebuffer[eye], cmd_buffer);

      graphene_matrix_t vp = _get_view_projection_matrix (self, eye);
      xrd_scene_window_draw (self->scene_window, eye,
                             self->pipelines[PIPELINE_WINDOWS],
                             self->pipeline_layout,
                             cmd_buffer, &vp);

      OpenVRContext *context = openvr_context_get_instance ();
      if (context->system->IsInputAvailable ())
        xrd_scene_pointer_render_pointer (self->scene_pointer,
                                          self->pipelines[PIPELINE_POINTER],
                                          cmd_buffer);

      _render_device_models (self, eye, cmd_buffer);
      vkCmdEndRenderPass (cmd_buffer);
    }
}

graphene_matrix_t
_get_hmd_pose_matrix (EVREye eye)
{
  graphene_matrix_t mat = openvr_system_get_eye_to_head_transform (eye);
  graphene_matrix_inverse (&mat, &mat);
  return mat;
}

graphene_matrix_t
_get_view_projection_matrix (OpenVRSceneRenderer *self,
                             EVREye               eye)
{
  graphene_matrix_t mat;
  graphene_matrix_init_from_matrix (&mat, &self->mat_head_pose);
  graphene_matrix_multiply (&mat, &self->mat_eye_pos[eye], &mat);
  graphene_matrix_multiply (&mat, &self->mat_projection[eye], &mat);
  return mat;
}

void
_update_device_poses (OpenVRSceneRenderer *self)
{
  OpenVRContext *context = openvr_context_get_instance ();
  context->compositor->WaitGetPoses (self->device_poses,
                                     k_unMaxTrackedDeviceCount, NULL, 0);

  for (uint32_t i = 0; i < k_unMaxTrackedDeviceCount; ++i)
    {
      if (!self->device_poses[i].bPoseIsValid)
        continue;

      openvr_math_matrix34_to_graphene (&self->device_poses[i].mDeviceToAbsoluteTracking,
                                        &self->device_mats[i]);
    }

  if (self->device_poses[k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
    {
      self->mat_head_pose = self->device_mats[k_unTrackedDeviceIndex_Hmd];
      graphene_matrix_inverse (&self->mat_head_pose, &self->mat_head_pose);
    }
}

bool
_init_shaders (OpenVRSceneRenderer *self)
{
  GulkanClient *client = GULKAN_CLIENT (self);

  const char *shader_names[PIPELINE_COUNT] = { "window", "pointer", "device_model" };
  const char *stage_names[2] = {"vert", "frag"};

  for (int32_t i = 0; i < PIPELINE_COUNT; i++)
    for (int32_t j = 0; j < 2; j++)
      {
        char path[1024];
        sprintf (path, "/shaders/%s.%s.spv", shader_names[i], stage_names[j]);

        if (!gulkan_renderer_create_shader_module (
            client->device->device, path,
           &self->shader_modules[i * 2 + j]))
          return false;
      }
  return true;
}

/* Create a descriptor set layout compatible with all shaders. */
bool
_init_descriptor_layout (OpenVRSceneRenderer *self)
{
  GulkanClient *client = GULKAN_CLIENT (self);

  VkDescriptorSetLayoutCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 2,
    .pBindings = (VkDescriptorSetLayoutBinding[]) {
      {
        .binding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
      },
      {
        .binding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
      }
    }
  };

  VkResult res;
  res = vkCreateDescriptorSetLayout (client->device->device,
                                    &info, NULL,
                                    &self->descriptor_set_layout);
  if (res != VK_SUCCESS)
    {
      g_print ("vkCreateDescriptorSetLayout failed with error %d\n", res);
      return false;
    }

  return true;
}

bool
_init_pipeline_layout (OpenVRSceneRenderer *self)
{
  GulkanClient *client = GULKAN_CLIENT (self);

  VkPipelineLayoutCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = &self->descriptor_set_layout,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges = NULL
  };

  VkResult res;
  res = vkCreatePipelineLayout (client->device->device,
                               &info, NULL, &self->pipeline_layout);
  if (res != VK_SUCCESS)
    {
      g_print ("vkCreatePipelineLayout failed with error %d\n", res);
      return false;
    }

  return true;
}

bool
_init_pipeline_cache (OpenVRSceneRenderer *self)
{
  GulkanClient *client = GULKAN_CLIENT (self);

  VkPipelineCacheCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO
  };
  VkResult res = vkCreatePipelineCache (client->device->device,
                                       &info, NULL, &self->pipeline_cache);
  if (res != VK_SUCCESS)
    {
      g_print ("vkCreatePipelineCache failed with error %d\n", res);
      return false;
    }

  return true;
}

typedef struct __attribute__((__packed__)) PipelineConfig {
  VkPrimitiveTopology                      topology;
  uint32_t                                 stride;
  const VkVertexInputAttributeDescription* attribs;
  uint32_t                                 attrib_count;
} PipelineConfig;

bool
_init_graphics_pipelines (OpenVRSceneRenderer *self)
{
  GulkanClient *client = GULKAN_CLIENT (self);

  PipelineConfig config[PIPELINE_COUNT] = {
    // PIPELINE_WINDOWS
    {
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .stride = sizeof (VertexDataScene),
      .attribs = (VkVertexInputAttributeDescription []) {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof (VertexDataScene, uv)},
      },
      .attrib_count = 2
    },
    // PIPELINE_POINTER
    {
      .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
      .stride = sizeof (float) * 6,
      .attribs = (VkVertexInputAttributeDescription []) {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof (float) * 3},
      },
      .attrib_count = 2
    },
    // PIPELINE_DEVICE_MODELS
    {
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .stride = sizeof (RenderModel_Vertex_t),
      .attribs = (VkVertexInputAttributeDescription []) {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof (RenderModel_Vertex_t, vNormal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof (RenderModel_Vertex_t, rfTextureCoord)},
      },
      .attrib_count = 3
    }
  };

  for (uint32_t i = 0; i < PIPELINE_COUNT; i++)
    {
      VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .layout = self->pipeline_layout,
        .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
          .pVertexAttributeDescriptions = config[i].attribs,
          .vertexBindingDescriptionCount = 1,
          .pVertexBindingDescriptions = &(VkVertexInputBindingDescription) {
            .binding = 0,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            .stride = config[i].stride
          },
          .vertexAttributeDescriptionCount = config[i].attrib_count
        },
        .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
          .topology = config[i].topology,
          .primitiveRestartEnable = VK_FALSE
        },
        .pViewportState = &(VkPipelineViewportStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
          .viewportCount = 1,
          .scissorCount = 1
        },
        .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
          .polygonMode = VK_POLYGON_MODE_FILL,
          .cullMode = VK_CULL_MODE_BACK_BIT,
          .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
          .lineWidth = 1.0f
        },
        .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
          .rasterizationSamples = self->msaa_sample_count,
          .minSampleShading = 0.0f,
          .pSampleMask = &(uint32_t) { 0xFFFFFFFF },
          .alphaToCoverageEnable = VK_FALSE
        },
        .pDepthStencilState = &(VkPipelineDepthStencilStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
          .depthTestEnable = VK_TRUE ,
          .depthWriteEnable = VK_TRUE,
          .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
          .depthBoundsTestEnable = VK_FALSE,
          .stencilTestEnable = VK_FALSE,
          .minDepthBounds = 0.0f,
          .maxDepthBounds = 0.0f
        },
        .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
          .logicOpEnable = VK_FALSE,
          .logicOp = VK_LOGIC_OP_COPY,
          .attachmentCount = 1,
          .blendConstants = {0,0,0,0},
          .pAttachments = &(VkPipelineColorBlendAttachmentState) {
            .blendEnable = VK_FALSE,
            .colorWriteMask = 0xf
          },
        },
        .stageCount = 2,
        .pStages = (VkPipelineShaderStageCreateInfo []) {
          {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = self->shader_modules[i * 2],
            .pName = "main"
          },
          {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = self->shader_modules[i * 2 + 1],
            .pName = "main"
          }
        },
        .renderPass = self->framebuffer[EVREye_Eye_Left]->render_pass,
        .pDynamicState = &(VkPipelineDynamicStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
          .dynamicStateCount = 2,
          .pDynamicStates = (VkDynamicState[]) {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
          }
        },
        .subpass = VK_NULL_HANDLE
      };

      VkResult res = vkCreateGraphicsPipelines (client->device->device,
                                                self->pipeline_cache, 1,
                                               &pipeline_info,
                                                NULL, &self->pipelines[i]);
      if (res != VK_SUCCESS)
        {
          g_print ("vkCreateGraphicsPipelines failed with error %d\n", res);
          return false;
        }
    }

  return true;
}

void
_init_descriptor_pool (OpenVRSceneRenderer *self)
{
  GulkanClient *client = GULKAN_CLIENT (self);

  VkDescriptorPoolCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = DESCRIPTOR_SET_COUNT,
    .poolSizeCount = 2,
    .pPoolSizes = (VkDescriptorPoolSize[]) {
      {
        .descriptorCount = DESCRIPTOR_SET_COUNT,
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
      },
      {
        .descriptorCount = DESCRIPTOR_SET_COUNT,
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
      }
    }
  };

  vkCreateDescriptorPool (client->device->device,
                          &info, NULL, &self->descriptor_pool);
}

void
_init_descriptor_sets (OpenVRSceneRenderer *self)
{
  GulkanClient *client = GULKAN_CLIENT (self);

  for (int i = 0; i < DESCRIPTOR_SET_COUNT; i++)
    {
      VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = self->descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &self->descriptor_set_layout
      };

      vkAllocateDescriptorSets (client->device->device, &alloc_info,
                                &self->descriptor_sets[i]);
    }

  xrd_scene_window_init_descriptor_sets (self->scene_window,
                                         client->device,
                                         self->descriptor_set_layout);
}

