/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "scene-renderer.h"

#include <graphene.h>

#include <gulkan.h>
#include <gxr.h>

#include "scene-controller.h"
#include "scene-pointer-tip.h"
#include "scene-object.h"

#include "graphene-ext.h"

#if defined(RENDERDOC)
#include <dlfcn.h>
#include "renderdoc_app.h"
static RENDERDOC_API_1_1_2 *rdoc_api = NULL;

static void _init_renderdoc ()
{
  if (rdoc_api != NULL)
    return;

  void *mod = dlopen ("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD);
  if (mod)
    {
      pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
      int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&rdoc_api);
      if (ret != 1)
        g_debug ("Failed to init renderdoc");
    }
}
#endif

typedef struct {
  graphene_point3d_t position;
  graphene_point_t   uv;
} SceneVertex;

typedef struct {
  float position[4];
  float color[3];
  float radius;
} SceneLight;

typedef struct {
  SceneLight lights[2];
  int active_lights;
} SceneLights;

struct _SceneRenderer
{
  GulkanRenderer parent;

  VkSampleCountFlagBits sample_count;
  float render_scale;

  VkShaderModule shader_modules[PIPELINE_COUNT * 2];
  VkPipeline pipelines[PIPELINE_COUNT];
  VkDescriptorSetLayout descriptor_set_layout;
  VkPipelineLayout pipeline_layout;
  VkPipelineCache pipeline_cache;

  GulkanRenderPass *render_pass;

  gpointer scene_client;

  SceneLights lights;
  GulkanUniformBuffer *lights_buffer;

  GxrContext *context;

  void
  (*render) (VkCommandBuffer  cmd_buffer,
             VkPipelineLayout pipeline_layout,
             VkPipeline      *pipelines,
             gpointer         data);

  void (*update_lights) (gpointer data);
};

G_DEFINE_TYPE (SceneRenderer, scene_renderer, GULKAN_TYPE_RENDERER)

static void
scene_renderer_finalize (GObject *gobject);

static void
scene_renderer_class_init (SceneRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = scene_renderer_finalize;
}

static void
scene_renderer_init (SceneRenderer *self)
{
  self->render_scale = 1.0f;
  self->render = NULL;
  self->scene_client = NULL;

  self->lights.active_lights = 0;
  graphene_vec4_t position;
  graphene_vec4_init (&position, 0, 0, 0, 1);

  graphene_vec4_t color;
  graphene_vec4_init (&color,.078f, .471f, .675f, 1);

  for (uint32_t i = 0; i < 2; i++)
    {
      graphene_vec4_to_float (&position, self->lights.lights[i].position);
      graphene_vec4_to_float (&color, self->lights.lights[i].color);
      self->lights.lights[i].radius = 0.1f;
    }

  self->descriptor_set_layout = VK_NULL_HANDLE;
  self->pipeline_layout = VK_NULL_HANDLE;
  self->pipeline_cache = VK_NULL_HANDLE;

  self->context = NULL;
}

static void
scene_renderer_finalize (GObject *gobject)
{
  SceneRenderer *self = SCENE_RENDERER (gobject);

  GulkanContext *gc = gxr_context_get_gulkan (self->context);

  VkDevice device = gulkan_context_get_device_handle (gc);
  if (device != VK_NULL_HANDLE)
    vkDeviceWaitIdle (device);

  if (device != VK_NULL_HANDLE)
    {
      g_clear_object (&self->lights_buffer);

      vkDestroyPipelineLayout (device, self->pipeline_layout, NULL);
      vkDestroyDescriptorSetLayout (device, self->descriptor_set_layout, NULL);
      for (uint32_t i = 0; i < PIPELINE_COUNT; i++)
        vkDestroyPipeline (device, self->pipelines[i], NULL);

      for (uint32_t i = 0; i < G_N_ELEMENTS (self->shader_modules); i++)
        vkDestroyShaderModule (device, self->shader_modules[i], NULL);

      vkDestroyPipelineCache (device, self->pipeline_cache, NULL);

      if (self->render_pass)
        g_clear_object (&self->render_pass);
    }

  g_clear_object (&self->context);

  G_OBJECT_CLASS (scene_renderer_parent_class)->finalize (gobject);
}

SceneRenderer *
scene_renderer_new (void)
{
  return (SceneRenderer*) g_object_new (SCENE_TYPE_RENDERER, 0);
}

static gboolean
_init_framebuffers (SceneRenderer *self)
{
  VkExtent2D extent;
  gxr_context_get_render_dimensions (self->context, &extent);

  extent.width = (uint32_t) (self->render_scale * (float) extent.width);
  extent.height = (uint32_t) (self->render_scale * (float) extent.height);

  gulkan_renderer_set_extent (GULKAN_RENDERER (self), extent);

  if (!gxr_context_init_framebuffers (self->context, extent, self->sample_count,
                                      &self->render_pass))
    return FALSE;

  return TRUE;
}

static gboolean
_init_shaders (SceneRenderer *self)
{
  const char *shader_names[PIPELINE_COUNT] = {
    "window", "window", "pointer", "pointer", "pointer"
  };
  const char *stage_names[2] = {"vert", "frag"};

  for (int32_t i = 0; i < PIPELINE_COUNT; i++)
    for (int32_t j = 0; j < 2; j++)
      {
        char path[1024];
        sprintf (path, "/shaders/%s.%s.spv", shader_names[i], stage_names[j]);

        if (!gulkan_renderer_create_shader_module (GULKAN_RENDERER (self),
                                                   path,
                                                  &self->shader_modules[i * 2 + j]))
          return FALSE;
      }
  return TRUE;
}

/* Create a descriptor set layout compatible with all shaders. */
static gboolean
_init_descriptor_layout (SceneRenderer *self)
{
  VkDescriptorSetLayoutCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 4,
    .pBindings = (VkDescriptorSetLayoutBinding[]) {
      // mvp buffer
      {
        .binding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
      },
      // Window and device texture
      {
        .binding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
      },
      // Window buffer
      {
        .binding = 2,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
      },
      // Lights buffer
      {
        .binding = 3,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
      },
    }
  };

  GulkanContext *gc = gxr_context_get_gulkan (self->context);
  VkDevice device = gulkan_context_get_device_handle (gc);
  VkResult res = vkCreateDescriptorSetLayout (device,
                                             &info, NULL,
                                             &self->descriptor_set_layout);
  vk_check_error ("vkCreateDescriptorSetLayout", res, FALSE);

  return TRUE;
}

static gboolean
_init_pipeline_layout (SceneRenderer *self)
{
  VkPipelineLayoutCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = &self->descriptor_set_layout,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges = NULL
  };

  GulkanContext *gc = gxr_context_get_gulkan (self->context);

  VkResult res = vkCreatePipelineLayout (gulkan_context_get_device_handle (gc),
                                        &info, NULL, &self->pipeline_layout);
  vk_check_error ("vkCreatePipelineLayout", res, FALSE);

  return TRUE;
}

static gboolean
_init_pipeline_cache (SceneRenderer *self)
{
  VkPipelineCacheCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO
  };

  GulkanContext *gc = gxr_context_get_gulkan (self->context);

  VkResult res = vkCreatePipelineCache (gulkan_context_get_device_handle (gc),
                                       &info, NULL, &self->pipeline_cache);
  vk_check_error ("vkCreatePipelineCache", res, FALSE);

  return TRUE;
}

typedef struct __attribute__((__packed__)) {
  VkPrimitiveTopology                           topology;
  uint32_t                                      stride;
  const VkVertexInputAttributeDescription      *attribs;
  uint32_t                                      attrib_count;
  const VkPipelineDepthStencilStateCreateInfo  *depth_stencil_state;
  const VkPipelineColorBlendAttachmentState    *blend_attachments;
  const VkPipelineRasterizationStateCreateInfo *rasterization_state;
} XrdPipelineConfig;

static gboolean
_init_graphics_pipelines (SceneRenderer *self)
{
  XrdPipelineConfig config[PIPELINE_COUNT] = {
    // PIPELINE_WINDOWS
    {
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .stride = sizeof (SceneVertex),
      .attribs = (VkVertexInputAttributeDescription []) {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof (SceneVertex, uv)},
      },
      .attrib_count = 2,
      .depth_stencil_state = &(VkPipelineDepthStencilStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
          .depthTestEnable = VK_TRUE,
          .depthWriteEnable = VK_TRUE,
          .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
      },
      .blend_attachments = &(VkPipelineColorBlendAttachmentState) {
        .blendEnable = VK_FALSE,
        .colorWriteMask = 0xf
      },
      .rasterization_state = &(VkPipelineRasterizationStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
          .polygonMode = VK_POLYGON_MODE_FILL,
          .cullMode = VK_CULL_MODE_BACK_BIT,
          .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
          .lineWidth = 1.0f
      }
    },
    // PIPELINE_TIP
    {
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .stride = sizeof (SceneVertex),
      .attribs = (VkVertexInputAttributeDescription []) {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof (SceneVertex, uv)},
      },
      .attrib_count = 2,
      .depth_stencil_state = &(VkPipelineDepthStencilStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
          .depthTestEnable = VK_FALSE,
          .depthWriteEnable = VK_FALSE
      },
      .blend_attachments = &(VkPipelineColorBlendAttachmentState) {
        .blendEnable = VK_TRUE,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT,
      },
      .rasterization_state = &(VkPipelineRasterizationStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
          .polygonMode = VK_POLYGON_MODE_FILL,
          .cullMode = VK_CULL_MODE_BACK_BIT,
          .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE
      }
    },
    // PIPELINE_POINTER
    {
      .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
      .stride = sizeof (float) * 6,
      .attribs = (VkVertexInputAttributeDescription []) {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof (float) * 3},
      },
      .depth_stencil_state = &(VkPipelineDepthStencilStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
          .depthTestEnable = VK_TRUE,
          .depthWriteEnable = VK_TRUE,
          .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
      },
      .attrib_count = 2,
      .blend_attachments = &(VkPipelineColorBlendAttachmentState) {
        .blendEnable = VK_FALSE,
        .colorWriteMask = 0xf
      },
      .rasterization_state = &(VkPipelineRasterizationStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
          .polygonMode = VK_POLYGON_MODE_LINE,
          .cullMode = VK_CULL_MODE_BACK_BIT,
          .lineWidth = 4.0f
      }
    },
    // PIPELINE_SELECTION
    {
      .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
      .stride = sizeof (float) * 6,
      .attribs = (VkVertexInputAttributeDescription []) {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof (float) * 3},
      },
      .depth_stencil_state = &(VkPipelineDepthStencilStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
          .depthTestEnable = VK_TRUE,
          .depthWriteEnable = VK_TRUE,
          .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
      },
      .attrib_count = 2,
      .blend_attachments = &(VkPipelineColorBlendAttachmentState) {
        .blendEnable = VK_FALSE,
        .colorWriteMask = 0xf
      },
      .rasterization_state = &(VkPipelineRasterizationStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
          .polygonMode = VK_POLYGON_MODE_LINE,
          .lineWidth = 2.0f
      }
    },
    // PIPELINE_BACKGROUND
    {
      .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
      .stride = sizeof (float) * 6,
      .attribs = (VkVertexInputAttributeDescription []) {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof (float) * 3},
      },
      .depth_stencil_state = &(VkPipelineDepthStencilStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
          .depthTestEnable = VK_TRUE,
          .depthWriteEnable = VK_TRUE,
          .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
      },
      .attrib_count = 2,
      .blend_attachments = &(VkPipelineColorBlendAttachmentState) {
        .blendEnable = VK_FALSE,
        .colorWriteMask = 0xf
      },
      .rasterization_state = &(VkPipelineRasterizationStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
          .polygonMode = VK_POLYGON_MODE_LINE,
          .lineWidth = 1.0f
      }
    },
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
        .pRasterizationState = config[i].rasterization_state,
        .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
          .rasterizationSamples = self->sample_count,
          .minSampleShading = 0.0f,
          .pSampleMask = &(uint32_t) { 0xFFFFFFFF },
          .alphaToCoverageEnable = VK_FALSE
        },
        .pDepthStencilState = config[i].depth_stencil_state,
        .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
          .logicOpEnable = VK_FALSE,
          .attachmentCount = 1,
          .blendConstants = {0,0,0,0},
          .pAttachments = config[i].blend_attachments,
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
        .renderPass = gulkan_render_pass_get_handle (self->render_pass),
        .pDynamicState = &(VkPipelineDynamicStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
          .dynamicStateCount = 2,
          .pDynamicStates = (VkDynamicState[]) {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
          }
        },
        .subpass = 0
      };

      GulkanContext *gc = gxr_context_get_gulkan (self->context);

      VkDevice device = gulkan_context_get_device_handle (gc);
      VkResult res;
      res = vkCreateGraphicsPipelines (device, self->pipeline_cache, 1,
                                      &pipeline_info, NULL,
                                      &self->pipelines[i]);
      vk_check_error ("vkCreateGraphicsPipelines", res, FALSE);
    }

  return TRUE;
}

gboolean
scene_renderer_init_vulkan (SceneRenderer *self,
                            GxrContext    *context)
{
  if (self->context)
    {
      g_warning ("Scene Renderer: Vulkan already initialized.\n");
      g_object_unref (self->context);
    }

  self->context = context;
  g_object_ref (self->context);

  gulkan_renderer_set_context (GULKAN_RENDERER (self),
                               gxr_context_get_gulkan (context));

  self->sample_count = VK_SAMPLE_COUNT_1_BIT;

  if (!_init_framebuffers (self))
    return FALSE;

  if (!_init_shaders (self))
    return FALSE;


  GulkanContext *gc = gxr_context_get_gulkan (context);
  GulkanDevice *device = gulkan_context_get_device (gc);

  self->lights_buffer =
    gulkan_uniform_buffer_new (device, sizeof (SceneLights));

  if (!self->lights_buffer)
    return FALSE;

  if (!_init_descriptor_layout (self))
    return FALSE;
  if (!_init_pipeline_layout (self))
    return FALSE;
  if (!_init_pipeline_cache (self))
    return FALSE;
  if (!_init_graphics_pipelines (self))
    return FALSE;

  return TRUE;
}

VkDescriptorSetLayout *
scene_renderer_get_descriptor_set_layout (SceneRenderer *self)
{
  return &self->descriptor_set_layout;
}

static void
_render_stereo (SceneRenderer *self, VkCommandBuffer cmd_buffer)
{
  VkExtent2D extent = gulkan_renderer_get_extent (GULKAN_RENDERER (self));

  VkViewport viewport = {
    .x = 0.0f,
    .y = (float) extent.height,
    .width = (float) extent.width,
    .height = - (float) extent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };
  vkCmdSetViewport (cmd_buffer, 0, 1, &viewport);
  VkRect2D scissor = {
    .offset = {0, 0},
    .extent = extent
  };
  vkCmdSetScissor (cmd_buffer, 0, 1, &scissor);

  VkClearColorValue black = {
    .float32 = { 0.0f, 0.0f, 0.0f, .0f },
  };

  GulkanFrameBuffer *framebuffer =
    gxr_context_get_acquired_framebuffer (self->context);

  if (!GULKAN_IS_FRAME_BUFFER (framebuffer))
    {
      g_printerr ("framebuffer invalid\n");
    }

  gulkan_render_pass_begin (self->render_pass, extent, black,
                            framebuffer, cmd_buffer);

  if (self->render)
    self->render (cmd_buffer, self->pipeline_layout,
                  self->pipelines, self->scene_client);

  vkCmdEndRenderPass (cmd_buffer);
}

void
scene_renderer_update_lights (SceneRenderer *self,
                              GList         *controllers)
{
  self->lights.active_lights = (int) g_list_length (controllers);
  if (self->lights.active_lights > 2)
    {
      g_warning ("Update lights received more than 2 controllers.\n");
      self->lights.active_lights = 2;
    }

  for (int i = 0; i < self->lights.active_lights; i++)
    {
      SceneController *controller =
        SCENE_CONTROLLER (g_list_nth_data (controllers, (guint) i));

      ScenePointerTip *scene_tip =
        SCENE_POINTER_TIP (scene_controller_get_pointer_tip (controller));

      graphene_point3d_t tip_position;
      scene_object_get_position (SCENE_OBJECT (scene_tip), &tip_position);

      self->lights.lights[i].position[0] = tip_position.x;
      self->lights.lights[i].position[1] = tip_position.y;
      self->lights.lights[i].position[2] = tip_position.z;
    }

  gulkan_uniform_buffer_update (self->lights_buffer,
                                (gpointer) &self->lights);
}

static void
_draw (SceneRenderer *self)
{
#if defined(RENDERDOC)
  if (rdoc_api) rdoc_api->StartFrameCapture(NULL, NULL);
  else _init_renderdoc ();
#endif

  GulkanContext *gc = gxr_context_get_gulkan (self->context);
  GulkanDevice *device = gulkan_context_get_device (gc);

  GulkanQueue *queue = gulkan_device_get_graphics_queue (device);

  GulkanCmdBuffer *cmd_buffer = gulkan_queue_request_cmd_buffer (queue);
  gulkan_cmd_buffer_begin_one_time (cmd_buffer);

  self->update_lights (self->scene_client);

  VkCommandBuffer cmd_handle = gulkan_cmd_buffer_get_handle (cmd_buffer);

  _render_stereo (self, cmd_handle);

  gulkan_queue_end_submit (queue, cmd_buffer);

  gulkan_queue_free_cmd_buffer (queue, cmd_buffer);

#if defined(RENDERDOC)
  if(rdoc_api) rdoc_api->EndFrameCapture(NULL, NULL);
#endif
}

gboolean
scene_renderer_draw (SceneRenderer *self)
{
  _draw (self);
  return TRUE;
}

void
scene_renderer_set_render_cb (SceneRenderer *self,
                              void (*render) (VkCommandBuffer  cmd_buffer,
                                              VkPipelineLayout pipeline_layout,
                                              VkPipeline      *pipelines,
                                              gpointer         data),
                              gpointer scene_client)
{
  self->render = render;
  self->scene_client = scene_client;
}

void
scene_renderer_set_update_lights_cb (SceneRenderer *self,
                                     void (*update_lights) (gpointer data),
                                     gpointer scene_client)
{
  self->update_lights = update_lights;
  self->scene_client = scene_client;
}

VkBuffer
scene_renderer_get_lights_buffer_handle (SceneRenderer *self)
{
  return gulkan_uniform_buffer_get_handle (self->lights_buffer);
}

GulkanContext *
scene_renderer_get_gulkan (SceneRenderer *self)
{
  return gxr_context_get_gulkan (self->context);
}

GulkanRenderPass *
scene_renderer_get_render_pass (SceneRenderer *self)
{
  return self->render_pass;
}
