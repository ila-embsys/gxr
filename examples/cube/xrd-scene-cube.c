/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gulkan.h>
#include "graphene-ext.h"
#include "xrd-scene-cube.h"

typedef struct
{
  float mv_matrix[16];
  float mvp_matrix[16];
  float normal_matrix[12];
} XrdSceneCubeUniformBuffer;

typedef struct {
  const gchar *vert;
  const gchar *frag;
} ShaderResources;

static const float positions[] = {
  // front
  -1.0f, +1.0f, +1.0f, // point blue
  +1.0f, +1.0f, +1.0f, // point magenta
  -1.0f, -1.0f, +1.0f, // point cyan
  +1.0f, -1.0f, +1.0f, // point white
  // back
  +1.0f, +1.0f, -1.0f, // point red
  -1.0f, +1.0f, -1.0f, // point black
  +1.0f, -1.0f, -1.0f, // point yellow
  -1.0f, -1.0f, -1.0f, // point green
  // right
  +1.0f, +1.0f, +1.0f, // point magenta
  +1.0f, +1.0f, -1.0f, // point red
  +1.0f, -1.0f, +1.0f, // point white
  +1.0f, -1.0f, -1.0f, // point yellow
  // left
  -1.0f, +1.0f, -1.0f, // point black
  -1.0f, +1.0f, +1.0f, // point blue
  -1.0f, -1.0f, -1.0f, // point green
  -1.0f, -1.0f, +1.0f, // point cyan
  // bottom
  -1.0f, -1.0f, +1.0f, // point cyan
  +1.0f, -1.0f, +1.0f, // point white
  -1.0f, -1.0f, -1.0f, // point green
  +1.0f, -1.0f, -1.0f, // point yellow
  // top
  -1.0f, +1.0f, -1.0f, // point black
  +1.0f, +1.0f, -1.0f, // point red
  -1.0f, +1.0f, +1.0f, // point blue
  +1.0f, +1.0f, +1.0f  // point magenta
};

static const float colors[] = {
  // front
  0.0f, 0.0f, 1.0f, // blue
  1.0f, 0.0f, 1.0f, // magenta
  0.0f, 1.0f, 1.0f, // cyan
  1.0f, 1.0f, 1.0f, // white
  // back
  1.0f, 0.0f, 0.0f, // red
  0.0f, 0.0f, 0.0f, // black
  1.0f, 1.0f, 0.0f, // yellow
  0.0f, 1.0f, 0.0f, // green
  // right
  1.0f, 0.0f, 1.0f, // magenta
  1.0f, 0.0f, 0.0f, // red
  1.0f, 1.0f, 1.0f, // white
  1.0f, 1.0f, 0.0f, // yellow
  // left
  0.0f, 0.0f, 0.0f, // black
  0.0f, 0.0f, 1.0f, // blue
  0.0f, 1.0f, 0.0f, // green
  0.0f, 1.0f, 1.0f, // cyan
  // bottom
  0.0f, 1.0f, 1.0f, // cyan
  1.0f, 1.0f, 1.0f, // white
  0.0f, 1.0f, 0.0f, // green
  1.0f, 1.0f, 0.0f, // yellow
  // top
  0.0f, 0.0f, 0.0f, // black
  1.0f, 0.0f, 0.0f, // red
  0.0f, 0.0f, 1.0f, // blue
  1.0f, 0.0f, 1.0f  // magenta
};

static const float normals[] = {
  // front
  +0.0f, +0.0f, +1.0f, // forward
  +0.0f, +0.0f, +1.0f, // forward
  +0.0f, +0.0f, +1.0f, // forward
  +0.0f, +0.0f, +1.0f, // forward
  // back
  +0.0f, +0.0f, -1.0f, // backbard
  +0.0f, +0.0f, -1.0f, // backbard
  +0.0f, +0.0f, -1.0f, // backbard
  +0.0f, +0.0f, -1.0f, // backbard
  // right
  +1.0f, +0.0f, +0.0f, // right
  +1.0f, +0.0f, +0.0f, // right
  +1.0f, +0.0f, +0.0f, // right
  +1.0f, +0.0f, +0.0f, // right
  // left
  -1.0f, +0.0f, +0.0f, // left
  -1.0f, +0.0f, +0.0f, // left
  -1.0f, +0.0f, +0.0f, // left
  -1.0f, +0.0f, +0.0f, // left
  // bottom
  +0.0f, -1.0f, +0.0f, // up
  +0.0f, -1.0f, +0.0f, // up
  +0.0f, -1.0f, +0.0f, // up
  +0.0f, -1.0f, +0.0f, // up
  // top
  +0.0f, +1.0f, +0.0f, // down
  +0.0f, +1.0f, +0.0f, // down
  +0.0f, +1.0f, +0.0f, // down
  +0.0f, +1.0f, +0.0f  // down
};

struct _XrdSceneCube
{
  XrdSceneObject parent;

  GulkanVertexBuffer *vb;
  GulkanClient *gulkan;
  GulkanRenderer *renderer;
  VkSampleCountFlagBits sample_count;

  VkDescriptorSetLayout descriptor_set_layout;

  VkPipeline pipeline;

  VkPipelineLayout pipeline_layout;
};

G_DEFINE_TYPE (XrdSceneCube, xrd_scene_cube, XRD_TYPE_SCENE_OBJECT)

static void
xrd_scene_cube_finalize (GObject *gobject);

static void
xrd_scene_cube_class_init (XrdSceneCubeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_cube_finalize;
}

static gboolean
_init_pipeline (XrdSceneCube *self,
                GulkanRenderPass *render_pass,
                gconstpointer  data)
{
  const ShaderResources *resources = (const ShaderResources*) data;

  VkDevice device = gulkan_client_get_device_handle (self->gulkan);

  VkPipelineVertexInputStateCreateInfo vi_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 3,
    .pVertexBindingDescriptions =
      (VkVertexInputBindingDescription[]){
        { .binding = 0,
          .stride = 3 * sizeof (float),
          .inputRate = VK_VERTEX_INPUT_RATE_VERTEX },
        { .binding = 1,
          .stride = 3 * sizeof (float),
          .inputRate = VK_VERTEX_INPUT_RATE_VERTEX },
        { .binding = 2,
          .stride = 3 * sizeof (float),
          .inputRate = VK_VERTEX_INPUT_RATE_VERTEX } },
    .vertexAttributeDescriptionCount = 3,
    .pVertexAttributeDescriptions =
      (VkVertexInputAttributeDescription[]){
        { .location = 0,
          .binding = 0,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = 0 },
        { .location = 1,
          .binding = 1,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = 0 },
        { .location = 2,
          .binding = 2,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = 0 } }
  };

  VkShaderModule vs_module;
  if (!gulkan_renderer_create_shader_module (
        self->renderer, resources->vert, &vs_module))
    return FALSE;

  VkShaderModule fs_module;
  if (!gulkan_renderer_create_shader_module (
        self->renderer, resources->frag, &fs_module))
    return FALSE;

  VkRenderPass pass = gulkan_render_pass_get_handle (render_pass);

  VkGraphicsPipelineCreateInfo pipeline_info = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = 2,
    .pStages =
      (VkPipelineShaderStageCreateInfo[]){
        {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = vs_module,
          .pName = "main",
        },
        {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = fs_module,
          .pName = "main",
        },
      },
    .pVertexInputState = &vi_create_info,
    .pInputAssemblyState =
      &(VkPipelineInputAssemblyStateCreateInfo){
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
      },
    .pViewportState = &(VkPipelineViewportStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1,
    },
    .pDynamicState =
      &(VkPipelineDynamicStateCreateInfo){
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates =
          (VkDynamicState[]){
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
         },
     },
    .pRasterizationState =
      &(VkPipelineRasterizationStateCreateInfo){
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f,
      },
    .pDepthStencilState =
      &(VkPipelineDepthStencilStateCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
      },
    .pMultisampleState =
      &(VkPipelineMultisampleStateCreateInfo){
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = self->sample_count,
      },
    .pColorBlendState =
      &(VkPipelineColorBlendStateCreateInfo){
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .blendConstants = { 0.f, 0.f, 0.f, 0.f },
        .pAttachments =
          (VkPipelineColorBlendAttachmentState[]){
            { .colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT },
          } },
    .layout = self->pipeline_layout,
    .renderPass = pass,
  };

  VkResult res = vkCreateGraphicsPipelines (
    device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &self->pipeline);

  vkDestroyShaderModule (device, vs_module, NULL);
  vkDestroyShaderModule (device, fs_module, NULL);

  vk_check_error ("vkCreateGraphicsPipelines", res, FALSE);

  return TRUE;
}

static gboolean
_init_pipeline_layout (XrdSceneCube *self)
{
  VkPipelineLayoutCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = &self->descriptor_set_layout,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges = NULL
  };

  VkDevice device = gulkan_client_get_device_handle (self->gulkan);

  VkResult res = vkCreatePipelineLayout (device, &info, NULL,
                                         &self->pipeline_layout);
  vk_check_error ("vkCreatePipelineLayout", res, FALSE);

  return TRUE;
}

static gboolean
_init_descriptor_set_layout (XrdSceneCube *self)
{
  VkDescriptorSetLayoutBinding bindings[] = {
    {
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .pImmutableSamplers = NULL,
    },
  };

  GulkanDevice *gulkan_device = gulkan_client_get_device (self->gulkan);

  VkDevice device = gulkan_device_get_handle (gulkan_device);

  VkDescriptorSetLayoutCreateInfo descriptor_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 1,
    .pBindings = bindings,
  };

  VkResult res;
  res = vkCreateDescriptorSetLayout (device, &descriptor_info,
                                     NULL, &self->descriptor_set_layout);
  vk_check_error ("vkCreateDescriptorSetLayout", res, FALSE);

  return TRUE;
}

static gboolean
_initialize (XrdSceneCube *self,
             GulkanClient *gulkan,
             GulkanRenderer *renderer,
             GulkanRenderPass *render_pass,
             VkSampleCountFlagBits sample_count)
{
  self->gulkan = g_object_ref (gulkan);
  self->renderer = g_object_ref (renderer);

  self->sample_count = sample_count;

  GulkanDevice *gulkan_device = gulkan_client_get_device (gulkan);

  self->vb = GULKAN_VERTEX_BUFFER_NEW_FROM_ATTRIBS (gulkan_device, positions,
                                                    colors, normals);

  if (!self->vb)
    return FALSE;

  const ShaderResources resources = {
    "/shaders/cube.vert.spv",
    "/shaders/cube.frag.spv"
  };

  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);

  if (!_init_descriptor_set_layout (self))
    return FALSE;

  if (!_init_pipeline_layout (self))
    return FALSE;

  if (!_init_pipeline (self, render_pass, (gconstpointer) &resources))
    return FALSE;

  VkDeviceSize ubo_size = sizeof (XrdSceneCubeUniformBuffer);
  if (!xrd_scene_object_initialize (obj, gulkan,
                                    &self->descriptor_set_layout, ubo_size))
    return FALSE;

  xrd_scene_object_update_descriptors (obj);

  return TRUE;
}

static void
xrd_scene_cube_init (XrdSceneCube *self)
{
  (void) self;
}

XrdSceneCube *
xrd_scene_cube_new (GulkanClient *gulkan,
                    GulkanRenderer *renderer,
                    GulkanRenderPass *render_pass,
                    VkSampleCountFlagBits sample_count)
{
  XrdSceneCube *self = (XrdSceneCube*) g_object_new (XRD_TYPE_SCENE_CUBE, 0);
  _initialize (self, gulkan, renderer, render_pass, sample_count);
  return self;
}

static void
xrd_scene_cube_finalize (GObject *gobject)
{
  XrdSceneCube *self = XRD_SCENE_CUBE (gobject);
  g_clear_object (&self->vb);
  g_clear_object (&self->renderer);
  g_clear_object (&self->gulkan);

  G_OBJECT_CLASS (xrd_scene_cube_parent_class)->finalize (gobject);
}

static void
_update_ubo (XrdSceneCube      *self,
             GxrEye             eye,
             graphene_matrix_t *view,
             graphene_matrix_t *projection)
{
  XrdSceneCubeUniformBuffer ub;

  graphene_matrix_t m_matrix;
  xrd_scene_object_get_transformation (XRD_SCENE_OBJECT (self), &m_matrix);

  graphene_matrix_t mv_matrix;
  graphene_matrix_multiply (&m_matrix, view, &mv_matrix);

  graphene_matrix_t mvp_matrix;
  graphene_matrix_multiply (&mv_matrix, projection, &mvp_matrix);

  graphene_matrix_to_float (&mv_matrix, ub.mv_matrix);
  graphene_matrix_to_float (&mvp_matrix, ub.mvp_matrix);

  /* The mat3 normalMatrix is laid out as 3 vec4s. */
  memcpy (ub.normal_matrix, ub.mv_matrix, sizeof ub.normal_matrix);

  xrd_scene_object_update_ubo (XRD_SCENE_OBJECT (self), eye, &ub);
}

static void
_set_transformation (XrdSceneCube *self)
{
  graphene_matrix_t m_matrix;
  graphene_matrix_init_identity (&m_matrix);

  int64_t t = gulkan_renderer_get_msec_since_start (self->renderer);
  t /= 5;

  graphene_matrix_rotate_x (&m_matrix, 45.0f + (0.25f * (float) t));
  graphene_matrix_rotate_y (&m_matrix, 45.0f - (0.5f * (float) t));
  graphene_matrix_rotate_z (&m_matrix, 10.0f + (0.15f * (float) t));

  graphene_point3d_t pos = { 0.0f, 0.0f, -6.0f };
  graphene_matrix_translate (&m_matrix, &pos);

  xrd_scene_object_set_transformation (XRD_SCENE_OBJECT (self), &m_matrix);
}

void
xrd_scene_cube_render (XrdSceneCube       *self,
                       GxrEye              eye,
                       VkCommandBuffer     cmd_buffer,
                       graphene_matrix_t  *view,
                       graphene_matrix_t  *projection)
{
  if (!gulkan_vertex_buffer_is_initialized (self->vb))
    {
      g_printerr ("Cube vb not initialized\n");
      return;
    }

  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);
  if (!xrd_scene_object_is_visible (obj))
    {
      // g_debug ("Cube not visible\n");
      return;
    }

  _set_transformation (self);

  /* TODO */
  vkCmdBindPipeline (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                     self->pipeline);
  /* TODO: would be nice if update_mvp matrix would fully update our ubo
   * xrd_scene_object_update_mvp_matrix (obj, eye, vp); */
  _update_ubo (self, eye, view, projection);

  xrd_scene_object_bind (obj, eye, cmd_buffer, self->pipeline_layout);

  gulkan_vertex_buffer_bind_with_offsets (self->vb, cmd_buffer);
  vkCmdDraw (cmd_buffer, 4, 1, 0, 0);
  vkCmdDraw (cmd_buffer, 4, 1, 4, 0);
  vkCmdDraw (cmd_buffer, 4, 1, 8, 0);
  vkCmdDraw (cmd_buffer, 4, 1, 12, 0);
  vkCmdDraw (cmd_buffer, 4, 1, 16, 0);
  vkCmdDraw (cmd_buffer, 4, 1, 20, 0);
}
