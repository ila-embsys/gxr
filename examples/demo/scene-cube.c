/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "scene-cube.h"
#include "graphene-ext.h"
#include <gulkan.h>

typedef struct __attribute__ ((__packed__))
{
  float mv_matrix[2][16];
  float mvp_matrix[2][16];
  float normal_matrix[2][12];
} SceneCubeUniformBuffer;

typedef struct
{
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

struct _SceneCube
{
  SceneObject parent;

  GulkanVertexBuffer   *vb;
  GulkanContext        *gulkan;
  GulkanRenderer       *renderer;
  VkSampleCountFlagBits sample_count;

  GulkanDescriptorPool *descriptor_pool;
  GulkanDescriptorSet  *descriptor_set;

  GulkanPipeline *pipeline;

  graphene_point3d_t pos;
};

G_DEFINE_TYPE (SceneCube, scene_cube, SCENE_TYPE_OBJECT)

static void
scene_cube_finalize (GObject *gobject);

static void
scene_cube_class_init (SceneCubeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = scene_cube_finalize;
}

static gboolean
_init_pipeline (SceneCube *self, GulkanRenderPass *render_pass)
{
  VkVertexInputBindingDescription *binding_desc
    = gulkan_vertex_buffer_create_binding_desc (self->vb);
  VkVertexInputAttributeDescription *attrib_desc
    = gulkan_vertex_buffer_create_attrib_desc (self->vb);

  GulkanPipelineConfig config = {
    .extent = gulkan_renderer_get_extent (self->renderer),
    .sample_count = VK_SAMPLE_COUNT_1_BIT,
    .vertex_shader_uri = "/shaders/cube.vert.spv",
    .fragment_shader_uri = "/shaders/cube.frag.spv",
    .topology = gulkan_vertex_buffer_get_topology (self->vb),
    .attribs = attrib_desc,
    .attrib_count = gulkan_vertex_buffer_get_attrib_count (self->vb),
    .bindings = binding_desc,
    .binding_count = gulkan_vertex_buffer_get_attrib_count (self->vb),
    .blend_attachments = (VkPipelineColorBlendAttachmentState[]){
      {
        .colorWriteMask =
          VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      },
    },
    .rasterization_state = &(VkPipelineRasterizationStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .lineWidth = 1.0f,
    },
    .depth_stencil_state = &(VkPipelineDepthStencilStateCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
      },
    .dynamic_viewport = FALSE,
    .flip_y = TRUE,
  };

  self->pipeline = gulkan_pipeline_new (self->gulkan, self->descriptor_pool,
                                        render_pass, &config);

  g_free (binding_desc);
  g_free (attrib_desc);

  return TRUE;
}

static gboolean
_init_descriptor_set_pool (SceneCube *self)
{
  VkDescriptorSetLayoutBinding bindings[] = {
    {
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .pImmutableSamplers = NULL,
    },
  };

  self->descriptor_pool = GULKAN_DESCRIPTOR_POOL_NEW (self->gulkan, bindings,
                                                      1);
  if (!self->descriptor_pool)
    return FALSE;

  return TRUE;
}

static gboolean
_initialize (SceneCube            *self,
             GulkanContext        *gulkan,
             GulkanRenderer       *renderer,
             GulkanRenderPass     *render_pass,
             VkSampleCountFlagBits sample_count)
{
  self->gulkan = g_object_ref (gulkan);
  self->renderer = g_object_ref (renderer);

  self->sample_count = sample_count;

  GulkanDevice *device = gulkan_context_get_device (gulkan);

  self->vb = gulkan_vertex_buffer_new (device,
                                       VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
  gulkan_vertex_buffer_add_attribute (self->vb, 3, sizeof (positions), 0,
                                      (uint8_t *) positions);
  gulkan_vertex_buffer_add_attribute (self->vb, 3, sizeof (colors), 0,
                                      (uint8_t *) colors);
  gulkan_vertex_buffer_add_attribute (self->vb, 3, sizeof (normals), 0,
                                      (uint8_t *) normals);

  if (!gulkan_vertex_buffer_upload (self->vb))
    return FALSE;

  if (!self->vb)
    return FALSE;

  SceneObject *obj = SCENE_OBJECT (self);

  if (!_init_descriptor_set_pool (self))
    return FALSE;

  if (!_init_pipeline (self, render_pass))
    return FALSE;

  self->descriptor_set
    = gulkan_descriptor_pool_create_set (self->descriptor_pool);

  VkDeviceSize ubo_size = sizeof (SceneCubeUniformBuffer);
  if (!scene_object_initialize (obj, gulkan, ubo_size, self->descriptor_set))
    return FALSE;

  scene_object_update_descriptors (obj);

  self->pos = (graphene_point3d_t){0.0f, 0.0f, -6.0f};

  return TRUE;
}

static void
scene_cube_init (SceneCube *self)
{
  (void) self;
}

SceneCube *
scene_cube_new (GulkanContext        *gulkan,
                GulkanRenderer       *renderer,
                GulkanRenderPass     *render_pass,
                VkSampleCountFlagBits sample_count)
{
  SceneCube *self = (SceneCube *) g_object_new (SCENE_TYPE_CUBE, 0);
  _initialize (self, gulkan, renderer, render_pass, sample_count);
  return self;
}

static void
scene_cube_finalize (GObject *gobject)
{
  SceneCube *self = SCENE_CUBE (gobject);

  g_object_unref (self->descriptor_set);
  g_object_unref (self->descriptor_pool);
  g_object_unref (self->pipeline);
  g_clear_object (&self->vb);
  g_clear_object (&self->renderer);
  g_clear_object (&self->gulkan);

  G_OBJECT_CLASS (scene_cube_parent_class)->finalize (gobject);
}

static void
_set_transformation (SceneCube *self)
{
  graphene_matrix_t m_matrix;
  graphene_matrix_init_identity (&m_matrix);

  int64_t t = gulkan_renderer_get_msec_since_start (self->renderer);
  t /= 5;

  graphene_matrix_rotate_x (&m_matrix, 45.0f + (0.25f * (float) t));
  graphene_matrix_rotate_y (&m_matrix, 45.0f - (0.5f * (float) t));
  graphene_matrix_rotate_z (&m_matrix, 10.0f + (0.15f * (float) t));

  graphene_matrix_translate (&m_matrix, &self->pos);

  scene_object_set_transformation (SCENE_OBJECT (self), &m_matrix);
}

void
scene_cube_render (SceneCube         *self,
                   VkCommandBuffer    cmd_buffer,
                   graphene_matrix_t *view,
                   graphene_matrix_t *projection)
{
  if (!gulkan_vertex_buffer_is_initialized (self->vb))
    {
      g_printerr ("Cube vb not initialized\n");
      return;
    }

  _set_transformation (self);

  gulkan_pipeline_bind (self->pipeline, cmd_buffer);
  /* TODO: would be nice if update_mvp matrix would fully update our ubo
   * scene_object_update_mvp_matrix (obj, eye, vp); */
  SceneCubeUniformBuffer ub;

  graphene_matrix_t m_matrix;
  scene_object_get_transformation (SCENE_OBJECT (self), &m_matrix);

  for (uint32_t eye = 0; eye < 2; eye++)
    {
      graphene_matrix_t mv_matrix;
      graphene_matrix_multiply (&m_matrix, &view[eye], &mv_matrix);

      graphene_matrix_t mvp_matrix;
      graphene_matrix_multiply (&mv_matrix, &projection[eye], &mvp_matrix);

      float mv[16];
      graphene_matrix_to_float (&mv_matrix, mv);
      for (int i = 0; i < 16; i++)
        ub.mv_matrix[eye][i] = mv[i];

      float mvp[16];
      graphene_matrix_to_float (&mvp_matrix, mvp);
      for (int i = 0; i < 16; i++)
        ub.mvp_matrix[eye][i] = mvp[i];

      /* The mat3 normalMatrix is laid out as 3 vec4s. */
      memcpy (ub.normal_matrix[eye], ub.mv_matrix[eye],
              sizeof ub.normal_matrix[eye]);
    }

  scene_object_update_ubo (SCENE_OBJECT (self), &ub);

  VkPipelineLayout layout
    = gulkan_descriptor_pool_get_pipeline_layout (self->descriptor_pool);

  gulkan_descriptor_set_bind (self->descriptor_set, layout, cmd_buffer);

  gulkan_vertex_buffer_bind_with_offsets (self->vb, cmd_buffer);
  vkCmdDraw (cmd_buffer, 4, 1, 0, 0);
  vkCmdDraw (cmd_buffer, 4, 1, 4, 0);
  vkCmdDraw (cmd_buffer, 4, 1, 8, 0);
  vkCmdDraw (cmd_buffer, 4, 1, 12, 0);
  vkCmdDraw (cmd_buffer, 4, 1, 16, 0);
  vkCmdDraw (cmd_buffer, 4, 1, 20, 0);
}

void
scene_cube_override_position (SceneCube *self, graphene_point3d_t *position)
{
  self->pos = *position;
}
