/*
 * gxr
 * Copyright 2021 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib-unix.h>
#include <glib.h>

#include <gxr.h>

#include <stdalign.h>

typedef struct
{
  alignas (32) float mvp[2][16];
} UniformBuffer;

// clang-format off
static const float positions[] = {
  -1.f, +1.f, +1.f,
  +1.f, +1.f, +1.f,
  -1.f, -1.f, +1.f,
  +1.f, -1.f, +1.f,
  +1.f, -1.f, -1.f,
  +1.f, +1.f, +1.f,
  +1.f, +1.f, -1.f,
  -1.f, +1.f, +1.f,
  -1.f, +1.f, -1.f,
  -1.f, -1.f, +1.f,
  -1.f, -1.f, -1.f,
  +1.f, -1.f, -1.f,
  -1.f, +1.f, -1.f,
  +1.f, +1.f, -1.f
};
// clang-format on

#define CUBE_TYPE_EXAMPLE cube_example_get_type ()
G_DECLARE_FINAL_TYPE (CubeExample, cube_example, CUBE, EXAMPLE, GulkanRenderer)

struct _CubeExample
{
  GulkanRenderer parent;

  GMainLoop *loop;

  GulkanPipeline       *pipeline;
  GulkanRenderPass     *render_pass;
  GulkanUniformBuffer  *uniform_buffer;
  GulkanDescriptorSet  *descriptor_set;
  GulkanDescriptorPool *descriptor_pool;
  GulkanVertexBuffer   *vb;
  GulkanCmdBuffer     **cmd_buffers;

  GxrContext *context;

  guint    render_source;
  guint    sigint_signal;
  gboolean render_background;

  UniformBuffer ub;
};

G_DEFINE_TYPE (CubeExample, cube_example, GULKAN_TYPE_RENDERER)

static void
_build_cmd_buffers (CubeExample *self);

static CubeExample *
cube_example_new (void)
{
  return (CubeExample *) g_object_new (CUBE_TYPE_EXAMPLE, 0);
}

static gboolean
_sigint_cb (gpointer _self)
{
  CubeExample *self = (CubeExample *) _self;
  g_main_loop_quit (self->loop);
  return TRUE;
}

static void
cube_example_init (CubeExample *self)
{
  self->context = NULL;
  self->descriptor_pool = NULL;
  self->loop = g_main_loop_new (NULL, FALSE);
  self->sigint_signal = g_unix_signal_add (SIGINT, _sigint_cb, self);
  self->render_background = TRUE;
  self->cmd_buffers = NULL;
}

static void
cube_example_finalize (GObject *gobject)
{
  CubeExample *self = CUBE_EXAMPLE (gobject);

  g_source_remove (self->render_source);
  g_source_remove (self->sigint_signal);

  GulkanContext *gc = gxr_context_get_gulkan (self->context);
  GulkanDevice  *gd = gulkan_context_get_device (gc);

  VkDevice     device = gulkan_context_get_device_handle (gc);
  GulkanQueue *queue = gulkan_device_get_graphics_queue (gd);

  vkDeviceWaitIdle (device);

  uint32_t swapchain_length = gxr_context_get_swapchain_length (self->context);
  for (uint32_t i = 0; i < swapchain_length; i++)
    {
      gulkan_queue_free_cmd_buffer (queue, self->cmd_buffers[i]);
    }

  g_free (self->cmd_buffers);

  g_object_unref (self->pipeline);

  g_object_unref (self->uniform_buffer);

  g_object_unref (self->descriptor_set);
  g_object_unref (self->descriptor_pool);
  g_clear_object (&self->vb);
  g_clear_object (&self->render_pass);
  g_clear_object (&self->context);

  g_main_loop_unref (self->loop);

  G_OBJECT_CLASS (cube_example_parent_class)->finalize (gobject);
}

static void
cube_example_class_init (CubeExampleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = cube_example_finalize;
}

static gboolean
_init_pipeline (CubeExample *self)
{
  VkVertexInputBindingDescription *binding_desc
    = gulkan_vertex_buffer_create_binding_desc (self->vb);
  VkVertexInputAttributeDescription *attrib_desc
    = gulkan_vertex_buffer_create_attrib_desc (self->vb);

  GulkanPipelineConfig config = {
    .extent = gulkan_renderer_get_extent (GULKAN_RENDERER (self)),
    .sample_count = VK_SAMPLE_COUNT_1_BIT,
    .vertex_shader_uri = "/shaders/minimal.vert.spv",
    .fragment_shader_uri = "/shaders/minimal.frag.spv",
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
    .depth_stencil_state =
      &(VkPipelineDepthStencilStateCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
      },
    .dynamic_viewport = FALSE,
    .flip_y = TRUE,
  };

  GulkanContext *gulkan = gxr_context_get_gulkan (self->context);
  self->pipeline = gulkan_pipeline_new (gulkan, self->descriptor_pool,
                                        self->render_pass, &config);

  g_free (binding_desc);
  g_free (attrib_desc);

  return TRUE;
}

static GxrContext *
_create_gxr_context ()
{
  GSList *instance_ext_list
    = gulkan_context_get_external_memory_instance_extensions ();

  GSList *device_ext_list
    = gulkan_context_get_external_memory_device_extensions ();

  device_ext_list
    = g_slist_append (device_ext_list,
                      g_strdup (VK_KHR_MAINTENANCE1_EXTENSION_NAME));

  GxrContext *context
    = gxr_context_new_from_vulkan_extensions (instance_ext_list,
                                              device_ext_list, "GXR Cube", 1);

  g_slist_free_full (instance_ext_list, g_free);
  g_slist_free_full (device_ext_list, g_free);
  return context;
}

static gboolean
_init_descriptor_pool (CubeExample *self)
{
  GulkanContext *gulkan = gxr_context_get_gulkan (self->context);

  VkDescriptorSetLayoutBinding bindings[] = {
    {
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .pImmutableSamplers = NULL,
    },
  };

  self->descriptor_pool = GULKAN_DESCRIPTOR_POOL_NEW (gulkan, bindings, 1);
  if (!self->descriptor_pool)
    return FALSE;

  self->descriptor_set
    = gulkan_descriptor_pool_create_set (self->descriptor_pool);

  return TRUE;
}

static gboolean
_init_vertex_buffer (CubeExample *self)
{
  GulkanContext *gc = gxr_context_get_gulkan (self->context);
  GulkanDevice  *device = gulkan_context_get_device (gc);
  self->vb = gulkan_vertex_buffer_new (device,
                                       VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
  gulkan_vertex_buffer_add_attribute (self->vb, 3, sizeof (positions), 0,
                                      (uint8_t *) positions);

  if (!gulkan_vertex_buffer_upload (self->vb))
    return FALSE;

  if (!self->vb)
    return FALSE;

  return TRUE;
}

static gboolean
_init_uniform_buffer (CubeExample *self)
{
  GulkanContext *gc = gxr_context_get_gulkan (self->context);
  GulkanDevice  *gulkan_device = gulkan_context_get_device (gc);
  /* Create uniform buffer to hold a matrix per eye */
  self->uniform_buffer = gulkan_uniform_buffer_new (gulkan_device,
                                                    sizeof (UniformBuffer));
  if (!self->uniform_buffer)
    return FALSE;

  return TRUE;
}

static void
_overlay_cb (GxrContext *context, GxrOverlayEvent *event, CubeExample *self)
{
  (void) context;
  self->render_background = !event->main_session_visible;
}

static gboolean
_init (CubeExample *self)
{
  self->context = _create_gxr_context ();
  if (!self->context)
    return FALSE;

  self->cmd_buffers
    = g_malloc (sizeof (GulkanCmdBuffer *)
                * gxr_context_get_swapchain_length (self->context));

  gulkan_renderer_set_context (GULKAN_RENDERER (self),
                               gxr_context_get_gulkan (self->context));

  VkExtent2D extent = gxr_context_get_swapchain_extent (self->context, 0);
  gulkan_renderer_set_extent (GULKAN_RENDERER (self), extent);

  if (!gxr_context_init_framebuffers (self->context, extent,
                                      VK_SAMPLE_COUNT_1_BIT,
                                      &self->render_pass))
    return FALSE;

  if (!_init_vertex_buffer (self))
    return FALSE;

  if (!_init_uniform_buffer (self))
    return FALSE;

  if (!_init_descriptor_pool (self))
    return FALSE;

  gulkan_descriptor_set_update_buffer (self->descriptor_set, 0,
                                       self->uniform_buffer);

  if (!_init_pipeline (self))
    return FALSE;

  _build_cmd_buffers (self);

  g_signal_connect (self->context, "overlay-event", (GCallback) _overlay_cb,
                    self);

  return TRUE;
}

static void
_update_transformation (CubeExample *self)
{
  graphene_matrix_t model;
  graphene_matrix_init_identity (&model);

  int64_t t = gulkan_renderer_get_msec_since_start (GULKAN_RENDERER (self));
  t /= 5;

  graphene_matrix_rotate_x (&model, 45.0f + (0.25f * (float) t));
  graphene_matrix_rotate_y (&model, 45.0f - (0.5f * (float) t));
  graphene_matrix_rotate_z (&model, 10.0f + (0.15f * (float) t));

  graphene_point3d_t pos = (graphene_point3d_t){0.0f, 0.0f, -6.0f};
  graphene_matrix_translate (&model, &pos);

  for (uint32_t eye = 0; eye < 2; eye++)
    {
      graphene_matrix_t view;
      graphene_matrix_t projection;

      gxr_context_get_view (self->context, eye, &view);
      gxr_context_get_projection (self->context, eye, 0.05f, 100.0f,
                                  &projection);

      graphene_matrix_t model_view;
      graphene_matrix_multiply (&model, &view, &model_view);

      graphene_matrix_t model_view_projection;
      graphene_matrix_multiply (&model_view, &projection,
                                &model_view_projection);
      graphene_matrix_to_float (&model_view_projection, self->ub.mvp[eye]);
    }

  gulkan_uniform_buffer_update (self->uniform_buffer, (gpointer) &self->ub);
}

static void
_render_stereo (CubeExample    *self,
                VkCommandBuffer cmd_buffer,
                uint32_t        swapchain_image_id)
{
  VkExtent2D extent = gulkan_renderer_get_extent (GULKAN_RENDERER (self));

  VkClearColorValue clear_color = {.float32 = {0, 0, 0, 0}};

  if (self->render_background)
    clear_color = (VkClearColorValue){
      .float32 = {0.1f, 0.1f, 0.7f, 1.0f},
    };

  GulkanFrameBuffer *framebuffer
    = gxr_context_get_framebuffer_at (self->context, swapchain_image_id);

  gulkan_render_pass_begin (self->render_pass, extent, clear_color, framebuffer,
                            cmd_buffer);

  gulkan_pipeline_bind (self->pipeline, cmd_buffer);

  VkPipelineLayout layout
    = gulkan_descriptor_pool_get_pipeline_layout (self->descriptor_pool);
  gulkan_descriptor_set_bind (self->descriptor_set, layout, cmd_buffer);

  gulkan_vertex_buffer_bind_with_offsets (self->vb, cmd_buffer);
  vkCmdDraw (cmd_buffer, 14, 1, 0, 0);

  vkCmdEndRenderPass (cmd_buffer);
}

static void
_build_cmd_buffers (CubeExample *self)
{
  GulkanContext *gc = gxr_context_get_gulkan (self->context);
  GulkanDevice  *device = gulkan_context_get_device (gc);
  GulkanQueue   *queue = gulkan_device_get_graphics_queue (device);

  uint32_t swapchain_length = gxr_context_get_swapchain_length (self->context);
  for (uint32_t i = 0; i < swapchain_length; i++)
    {
      self->cmd_buffers[i] = gulkan_queue_request_cmd_buffer (queue);
      gulkan_cmd_buffer_begin (self->cmd_buffers[i], 0);

      VkCommandBuffer cmd_handle = gulkan_cmd_buffer_get_handle (self
                                                                   ->cmd_buffers
                                                                     [i]);
      _render_stereo (self, cmd_handle, i);
      gulkan_cmd_buffer_end (self->cmd_buffers[i]);
    }
}

static gboolean
_iterate_cb (gpointer _self)
{
  CubeExample   *self = (CubeExample *) _self;
  GulkanContext *gc = gxr_context_get_gulkan (self->context);
  GulkanDevice  *device = gulkan_context_get_device (gc);
  GulkanQueue   *queue = gulkan_device_get_graphics_queue (device);

  gxr_context_poll_events (self->context);

  if (!gxr_context_begin_frame (self->context))
    return FALSE;

  _update_transformation (self);

  uint32_t i = gxr_context_get_buffer_index (self->context);

  gulkan_queue_submit (queue, self->cmd_buffers[i]);

  gxr_context_end_frame (self->context);

  return TRUE;
}

static void
_run (CubeExample *self)
{
  self->render_source = g_timeout_add (1, _iterate_cb, self);
  g_main_loop_run (self->loop);
}

int
main (void)
{
  CubeExample *self = cube_example_new ();
  if (!_init (self))
    return 1;

  _run (self);
  g_object_unref (self);

  return 0;
}
