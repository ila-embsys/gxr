/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib-unix.h>
#include <glib.h>

#include <gxr.h>

#include "scene-controller.h"

#include "gxr-device-manager.h"
#include "scene-background.h"
#include "scene-cube.h"
#include "scene-pointer.h"

#if defined(RENDERDOC)
#include "renderdoc_app.h"
#include <dlfcn.h>
static RENDERDOC_API_1_1_2 *rdoc_api = NULL;

static void
_init_renderdoc ()
{
  if (rdoc_api != NULL)
    return;

  void *mod = dlopen ("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD);
  if (mod)
    {
      pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)
        dlsym (mod, "RENDERDOC_GetAPI");
      int ret = RENDERDOC_GetAPI (eRENDERDOC_API_Version_1_1_2,
                                  (void **) &rdoc_api);
      if (ret != 1)
        g_debug ("Failed to init renderdoc");
    }
}
#endif

typedef struct
{
  graphene_point3d_t position;
  graphene_point_t   uv;
} SceneVertex;

#define GXR_TYPE_DEMO gxr_demo_get_type ()
G_DECLARE_FINAL_TYPE (GxrDemo, gxr_demo, GXR, DEMO, GulkanRenderer)

struct _GxrDemo
{
  GulkanRenderer parent;

  GMainLoop *loop;

  GxrController    *cube_grabbed;
  graphene_matrix_t pointer_pose;

  GxrContext *context;

  guint render_source;
  bool  shutdown;
  bool  rendering;
  bool  framecycle;

  GxrActionSet *actionset;
  guint         poll_input_source_id;

  guint poll_runtime_event_source_id;

  gulong device_activate_signal;
  gulong device_deactivate_signal;
  guint  sigint_signal;

  graphene_matrix_t mat_view[2];
  graphene_matrix_t mat_projection[2];

  float near;
  float far;

  SceneBackground *background;
  gboolean         render_background;
  SceneCube       *cube;

  // GxrController -> SceneController
  GHashTable *controller_map;

  VkSampleCountFlagBits sample_count;

  GulkanDescriptorPool *descriptor_pool;
  GulkanPipeline       *line_pipeline;

  GxrManifest *manifest;

  GulkanRenderPass *render_pass;
};

G_DEFINE_TYPE (GxrDemo, gxr_demo, GULKAN_TYPE_RENDERER)

static GxrDemo *
gxr_demo_new (void)
{
  return (GxrDemo *) g_object_new (GXR_TYPE_DEMO, 0);
}

static gboolean
_sigint_cb (gpointer _self)
{
  GxrDemo *self = (GxrDemo *) _self;
  g_main_loop_quit (self->loop);
  return TRUE;
}

static void
_finalize (GObject *gobject)
{
  GxrDemo *self = GXR_DEMO (gobject);
  self->shutdown = true;

  if (self->render_source > 0)
    g_source_remove (self->render_source);
  self->render_source = 0;

  if (self->poll_runtime_event_source_id > 0)
    g_source_remove (self->poll_runtime_event_source_id);
  self->poll_runtime_event_source_id = 0;

  if (self->poll_input_source_id > 0)
    g_source_remove (self->poll_input_source_id);
  self->poll_input_source_id = 0;

  GxrDeviceManager *dm = gxr_context_get_device_manager (self->context);
  g_signal_handler_disconnect (dm, self->device_activate_signal);
  g_signal_handler_disconnect (dm, self->device_deactivate_signal);

  g_source_remove (self->sigint_signal);

  g_clear_object (&self->actionset);
  g_clear_object (&self->manifest);
  g_clear_object (&self->background);
  g_clear_object (&self->cube);

  GulkanContext *gc = gxr_context_get_gulkan (self->context);
  VkDevice       device = gulkan_context_get_device_handle (gc);
  if (device != VK_NULL_HANDLE)
    vkDeviceWaitIdle (device);

  g_object_unref (self->descriptor_pool);
  g_object_unref (self->line_pipeline);
  g_clear_object (&self->render_pass);
  g_clear_object (&self->context);
  g_hash_table_unref (self->controller_map);

  g_print ("Cleaned up!\n");
}

static void
gxr_demo_class_init (GxrDemoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = _finalize;
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
                                              device_ext_list, "gxr Demo", 1);

  g_slist_free_full (instance_ext_list, g_free);
  g_slist_free_full (device_ext_list, g_free);
  return context;
}

static void
_render_pointers (GxrDemo           *self,
                  VkCommandBuffer    cmd_buffer,
                  VkPipelineLayout   pipeline_layout,
                  graphene_matrix_t *vp)
{
  GList *scs = g_hash_table_get_values (self->controller_map);
  for (GList *l = scs; l; l = l->next)
    {
      SceneController *sc = SCENE_CONTROLLER (l->data);

      if (!gxr_controller_is_pointer_pose_valid (
            scene_controller_get_controller (sc)))
        {
          /*
          g_print ("Pointer pose not valid: %lu\n",
                   scene_controller_get_handle (controller));
          */
          continue;
        }

      ScenePointer *pointer = SCENE_POINTER (scene_controller_get_pointer (sc));

      scene_pointer_render (pointer, self->line_pipeline, pipeline_layout,
                            cmd_buffer, vp);
    }
}

static void
_cube_set_position (GxrDemo *example)
{
  if (!example->cube_grabbed)
    return;

  graphene_matrix_t pointer_pose;
  gxr_controller_get_pointer_pose (GXR_CONTROLLER (example->cube_grabbed),
                                   &pointer_pose);

  SceneController *sc = g_hash_table_lookup (example->controller_map,
                                             example->cube_grabbed);
  ScenePointer    *pointer = scene_controller_get_pointer (sc);
  float            distance = scene_pointer_get_length (pointer);

  graphene_point3d_t p = {.x = .0f, .y = .0f, .z = -distance};
  graphene_matrix_transform_point3d (&pointer_pose, &p, &p);

  scene_cube_override_position (example->cube, &p);
}

static void
_render (GxrDemo *self, VkCommandBuffer cmd_buffer)
{
  graphene_matrix_t vp[2];

  for (uint32_t eye = 0; eye < 2; eye++)
    graphene_matrix_multiply (&self->mat_view[eye], &self->mat_projection[eye],
                              &vp[eye]);

  VkPipelineLayout layout
    = gulkan_descriptor_pool_get_pipeline_layout (self->descriptor_pool);

  if (self->render_background)
    scene_background_render (self->background, self->line_pipeline, layout,
                             cmd_buffer, vp);

  _render_pointers (self, cmd_buffer, layout, vp);

  _cube_set_position (self);
  scene_cube_render (self->cube, cmd_buffer, self->mat_view,
                     self->mat_projection);
}

static void
_render_stereo (GxrDemo *self, VkCommandBuffer cmd_buffer)
{
  VkExtent2D extent = gulkan_renderer_get_extent (GULKAN_RENDERER (self));

  VkClearColorValue black = {
    .float32 = {0.0f, 0.0f, 0.0f, .0f},
  };

  GulkanFrameBuffer *framebuffer
    = gxr_context_get_acquired_framebuffer (self->context);

  if (!GULKAN_IS_FRAME_BUFFER (framebuffer))
    {
      g_printerr ("framebuffer invalid\n");
    }

  gulkan_render_pass_begin (self->render_pass, extent, black, framebuffer,
                            cmd_buffer);

  _render (self, cmd_buffer);

  vkCmdEndRenderPass (cmd_buffer);
}

static void
_draw (GxrDemo *self)
{
#if defined(RENDERDOC)
  if (rdoc_api)
    rdoc_api->StartFrameCapture (NULL, NULL);
  else
    _init_renderdoc ();
#endif

  GulkanContext *gc = gxr_context_get_gulkan (self->context);
  GulkanDevice  *device = gulkan_context_get_device (gc);

  GulkanQueue *queue = gulkan_device_get_graphics_queue (device);

  GulkanCmdBuffer *cmd_buffer = gulkan_queue_request_cmd_buffer (queue);
  gulkan_cmd_buffer_begin_one_time (cmd_buffer);

  VkCommandBuffer cmd_handle = gulkan_cmd_buffer_get_handle (cmd_buffer);

  _render_stereo (self, cmd_handle);

  gulkan_queue_end_submit (queue, cmd_buffer);

  gulkan_queue_free_cmd_buffer (queue, cmd_buffer);

#if defined(RENDERDOC)
  if (rdoc_api)
    rdoc_api->EndFrameCapture (NULL, NULL);
#endif
}

static gboolean
_iterate_cb (gpointer _self)
{
  GxrDemo *self = (GxrDemo *) _self;

  if (self->shutdown)
    return FALSE;

  if (!self->framecycle)
    return TRUE;

  if (!gxr_context_begin_frame (self->context))
    return FALSE;

  for (uint32_t eye = 0; eye < 2; eye++)
    {
      gxr_context_get_view (self->context, eye, &self->mat_view[eye]);
      gxr_context_get_projection (self->context, eye, self->near, self->far,
                                  &self->mat_projection[eye]);
    }

  if (self->rendering)
    _draw (self);

  gxr_context_end_frame (self->context);

  return TRUE;
}

static void
_device_activate_cb (GxrDeviceManager *dm,
                     GxrController    *controller,
                     gpointer          _self)
{
  (void) dm;

  GxrDemo *self = _self;

  g_print ("Controller %lu activated.\n",
           gxr_device_get_handle (GXR_DEVICE (controller)));

  GulkanContext *client = gxr_context_get_gulkan (self->context);

  SceneController *sc = scene_controller_new (controller, self->context);

  GulkanDescriptorSet *descriptor_set
    = gulkan_descriptor_pool_create_set (self->descriptor_pool);

  ScenePointer *pointer = scene_pointer_new (client, descriptor_set);
  scene_controller_set_pointer (sc, pointer);

  g_hash_table_insert (self->controller_map, controller, sc);
}

static void
_device_deactivate_cb (GxrDeviceManager *dm,
                       GxrController    *controller,
                       gpointer          _self)
{
  (void) dm;
  GxrDemo *self = _self;
  g_print ("Controller deactivated..\n");

  SceneController *sc = g_hash_table_lookup (self->controller_map, controller);
  g_hash_table_remove (self->controller_map, controller);
  g_object_unref (sc);
}

static void
_action_quit_cb (GxrAction *action, GxrDigitalEvent *event, GxrDemo *self)
{
  (void) action;
  if (event->active && event->changed && event->state)
    {
      g_print ("Quitting example\n");
      g_main_loop_quit (self->loop);
    }
}

static void
_action_grab_cb (GxrAction *action, GxrDigitalEvent *event, GxrDemo *self)
{
  (void) action;
  if (event->active && event->changed && event->state)
    {
      if (self->cube_grabbed == NULL)
        {
          g_print ("Grabbing cube\n");
          self->cube_grabbed = event->controller;
        }
    }
  else if (event->active && event->changed && !event->state)
    {
      g_print ("Ungrabbing cube\n");
      self->cube_grabbed = NULL;
    }
}

static GxrActionSet *
_create_wm_action_set (GxrDemo *self)
{
  GxrActionSet *set = gxr_action_set_new_from_url (self->context,
                                                   self->manifest,
                                                   "/actions/wm");

  gxr_action_set_connect_digital_from_float (set, "/actions/wm/in/grab_window",
                                             0.25f, "/actions/wm/out/haptic",
                                             (GCallback) _action_grab_cb, self);

  gxr_action_set_connect (set, GXR_ACTION_DIGITAL, "/actions/wm/in/menu",
                          (GCallback) _action_quit_cb, self);

  GxrDeviceManager *dm = gxr_context_get_device_manager (self->context);

  gxr_device_manager_connect_pose_actions (dm, set, "/actions/wm/in/hand_pose",
                                           NULL);

  return set;
}

static gboolean
_poll_runtime_events (GxrDemo *self)
{
  if (!self->context)
    return FALSE;

  gxr_context_poll_events (self->context);

  return TRUE;
}

static gboolean
_poll_input_events (GxrDemo *self)
{
  if (!self->framecycle)
    return TRUE;

  if (!self->context)
    {
      g_printerr ("Error polling events: No Gxr Context\n");
      self->poll_input_source_id = 0;
      return FALSE;
    }

  if (self->actionset == NULL)
    {
      g_printerr ("Error: Action Set not created!\n");
      return TRUE;
    }

  if (!gxr_action_sets_poll (&self->actionset, 1))
    {
      g_printerr ("Error polling actions\n");
      self->poll_input_source_id = 0;
      return FALSE;
    }

  return TRUE;
}

static void
_init_input_callbacks (GxrDemo *self)
{

  self->manifest = gxr_manifest_new ("/res/bindings", "actions.json");

  if (!self->manifest)
    {
      g_print ("Failed to load action bindings!\n");
      return;
    }

  self->actionset = _create_wm_action_set (self);
  gxr_context_attach_action_sets (self->context, &self->actionset, 1);

  self->poll_input_source_id = g_timeout_add (3,
                                              (GSourceFunc) _poll_input_events,
                                              self);
}

static void
_state_change_cb (GxrContext          *context,
                  GxrStateChangeEvent *event,
                  GxrDemo             *self)
{

  (void) context;
  switch (event->state_change)
    {
      case GXR_STATE_SHUTDOWN:
        g_main_loop_quit (self->loop);
        break;
      case GXR_STATE_FRAMECYCLE_START:
        self->framecycle = TRUE;
        break;
      case GXR_STATE_FRAMECYCLE_STOP:
        self->framecycle = FALSE;
        break;
      case GXR_STATE_RENDERING_START:
        self->rendering = TRUE;
        break;
      case GXR_STATE_RENDERING_STOP:
        self->rendering = FALSE;
        break;
    }
}

static void
_overlay_cb (GxrContext *context, GxrOverlayEvent *event, GxrDemo *self)
{
  (void) context;
  self->render_background = !event->main_session_visible;
}

static gboolean
_init_framebuffers (GxrDemo *self)
{
  VkExtent2D extent = gxr_context_get_swapchain_extent (self->context, 0);

  gulkan_renderer_set_extent (GULKAN_RENDERER (self), extent);

  if (!gxr_context_init_framebuffers (self->context, extent, self->sample_count,
                                      &self->render_pass))
    return FALSE;

  return TRUE;
}

/* Create a descriptor set layout compatible with all shaders. */
static gboolean
_init_descriptor_pool (GxrDemo *self)
{
  VkDescriptorSetLayoutBinding bindings[] = {
    // mvp buffer
    {
      .binding = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    },
  };

  GulkanContext *gulkan = gxr_context_get_gulkan (self->context);
  self->descriptor_pool = GULKAN_DESCRIPTOR_POOL_NEW (gulkan, bindings, 8);
  if (!self->descriptor_pool)
    return FALSE;

  return TRUE;
}

static gboolean
_init_graphics_pipelines (GxrDemo *self)
{
  GulkanPipelineConfig config = {
    .extent = gulkan_renderer_get_extent (GULKAN_RENDERER (self)),
    .sample_count = VK_SAMPLE_COUNT_1_BIT,
    .vertex_shader_uri = "/shaders/pointer.vert.spv",
    .fragment_shader_uri = "/shaders/pointer.frag.spv",
    .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
    .attribs = (VkVertexInputAttributeDescription []) {
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof (float) * 3},
    },
    .attrib_count = 2,
    .bindings = &(VkVertexInputBindingDescription) {
        .binding = 0,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        .stride = sizeof (float) * 6,
    },
    .binding_count = 1,
    .blend_attachments = &(VkPipelineColorBlendAttachmentState) {
      .blendEnable = VK_FALSE,
      .colorWriteMask = 0xf,
    },
    .rasterization_state = &(VkPipelineRasterizationStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_LINE,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .lineWidth = 2.0f,
    },
    .depth_stencil_state = &(VkPipelineDepthStencilStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
    },
    .flip_y = TRUE,
  };

  GulkanContext *gulkan = gulkan_renderer_get_context (GULKAN_RENDERER (self));
  self->line_pipeline = gulkan_pipeline_new (gulkan, self->descriptor_pool,
                                             self->render_pass, &config);

  return TRUE;
}

static gboolean
_init_vulkan (GxrDemo *self)
{
  GulkanContext *gulkan = gxr_context_get_gulkan (self->context);

  gulkan_renderer_set_context (GULKAN_RENDERER (self), gulkan);

  self->sample_count = VK_SAMPLE_COUNT_1_BIT;

  if (!_init_framebuffers (self))
    return FALSE;
  if (!_init_descriptor_pool (self))
    return FALSE;
  if (!_init_graphics_pipelines (self))
    return FALSE;

  self->cube = scene_cube_new (gulkan, GULKAN_RENDERER (self),
                               self->render_pass, self->sample_count);

  GulkanDescriptorSet *descriptor_set
    = gulkan_descriptor_pool_create_set (self->descriptor_pool);
  self->background = scene_background_new (gulkan, descriptor_set);

  return TRUE;
}

static void
gxr_demo_init (GxrDemo *self)
{
  self->shutdown = false;

  self->poll_runtime_event_source_id = 0;
  self->poll_input_source_id = 0;
  self->actionset = NULL;
  self->context = NULL;
  self->near = 0.05f;
  self->far = 100.0f;
  self->background = NULL;
  self->cube_grabbed = NULL;
  self->device_activate_signal = 0;
  self->device_deactivate_signal = 0;
  self->manifest = NULL;
  graphene_matrix_init_identity (&self->pointer_pose);

  self->loop = g_main_loop_new (NULL, FALSE),

  self->render_source = g_timeout_add (1, _iterate_cb, self);

  self->sigint_signal = g_unix_signal_add (SIGINT, _sigint_cb, self);

  self->context = _create_gxr_context ();
  g_assert (self->context);

  self->controller_map = g_hash_table_new (g_direct_hash, g_direct_equal);

  GxrDeviceManager *dm = gxr_context_get_device_manager (self->context);
  self->device_activate_signal = g_signal_connect (dm, "device-activate-event",
                                                   (GCallback)
                                                     _device_activate_cb,
                                                   self);
  self->device_deactivate_signal = g_signal_connect (dm,
                                                     "device-deactivate-event",
                                                     (GCallback)
                                                       _device_deactivate_cb,
                                                     self);

  graphene_vec4_t position;
  graphene_vec4_init (&position, 0, 0, 0, 1);

  graphene_vec4_t color;
  graphene_vec4_init (&color, .078f, .471f, .675f, 1);

  self->render_background = TRUE;

  g_assert (_init_vulkan (self));

  _init_input_callbacks (self);

  self->poll_runtime_event_source_id = g_timeout_add (20,
                                                      (GSourceFunc)
                                                        _poll_runtime_events,
                                                      self);

  g_signal_connect (self->context, "state-change-event",
                    (GCallback) _state_change_cb, self);

  g_signal_connect (self->context, "overlay-event", (GCallback) _overlay_cb,
                    self);
}

int
main (void)
{
  GxrDemo *demo = gxr_demo_new ();
  g_main_loop_run (demo->loop);
  g_object_unref (demo);
  return 0;
}
