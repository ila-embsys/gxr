/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <glib-unix.h>


#include <gxr.h>

#include "graphene-ext.h"

#include "gxr-pointer-tip.h"
#include "gxr-controller.h"

#include "xrd-scene-pointer-tip.h"
#include "xrd-scene-renderer.h"
#include "xrd-scene-device-manager.h"
#include "xrd-scene-background.h"
#include "xrd-scene-cube.h"

typedef struct Example
{
  GMainLoop *loop;
  GxrContext *context;

  guint quit_source;
  guint render_source;
  bool shutdown;

  GxrActionSet *actionset_wm;
  guint poll_input_source_id;

  guint poll_runtime_event_source_id;

  XrdSceneDeviceManager *device_manager;

  graphene_matrix_t mat_view[2];
  graphene_matrix_t mat_projection[2];

  float near;
  float far;

  gboolean have_projection;

  XrdSceneBackground *background;

  XrdSceneCube *cube;
} Example;

static gboolean
_sigint_cb (gpointer _self)
{
  Example *self = (Example*) _self;
  g_main_loop_quit (self->loop);
  return TRUE;
}

static void
_cleanup (Example *self)
{
  self->shutdown = true;

  if (self->render_source > 0)
    g_source_remove (self->render_source);
  self->render_source = 0;

  if (self->quit_source > 0)
    g_source_remove (self->quit_source);
  self->quit_source = 0;

  if (self->poll_runtime_event_source_id > 0)
    g_source_remove (self->poll_runtime_event_source_id);
  self->poll_runtime_event_source_id = 0;

  if (self->poll_input_source_id > 0)
    g_source_remove (self->poll_input_source_id);
  self->poll_input_source_id = 0;

  g_clear_object (&self->actionset_wm);

  g_clear_object (&self->device_manager);
  g_object_unref (self->background);

  xrd_scene_renderer_destroy_instance ();

  g_clear_object (&self->context);

  g_print ("Cleaned up!\n");
}

static gboolean
_iterate_cb (gpointer _self)
{
  Example *self = (Example*) _self;

  if (self->shutdown)
    return FALSE;

  GxrContext *context = self->context;
  if (!gxr_context_begin_frame (context))
    return FALSE;

  for (uint32_t eye = 0; eye < 2; eye++)
    gxr_context_get_view (context, eye, &self->mat_view[eye]);


  for (uint32_t eye = 0; eye < 2; eye++)
    gxr_context_get_projection (context, eye, self->near, self->far,
                                &self->mat_projection[eye]);

  XrdSceneRenderer *renderer = xrd_scene_renderer_get_instance ();
  xrd_scene_renderer_draw (renderer);

  GxrPose poses[GXR_DEVICE_INDEX_MAX];
  gxr_context_end_frame (context, poses);

  xrd_scene_device_manager_update_poses (self->device_manager, poses);
  return TRUE;
}

static GxrContext *
_create_gxr_context ()
{
  GSList *instance_ext_list =
  gulkan_client_get_external_memory_instance_extensions ();

  GSList *device_ext_list =
  gulkan_client_get_external_memory_device_extensions ();

  device_ext_list = g_slist_append (device_ext_list,
                                    VK_KHR_MAINTENANCE1_EXTENSION_NAME);

  GxrContext *context = gxr_context_new_from_vulkan_extensions (GXR_APP_SCENE,
                                                                instance_ext_list,
                                                                device_ext_list);

  g_slist_free (instance_ext_list);
  g_slist_free (device_ext_list);
  return context;
}

static void
_init_device_model (Example *self,
                    uint32_t   device_id)
{
  GxrContext *context = self->context;

  XrdSceneRenderer *renderer = xrd_scene_renderer_get_instance ();
  VkDescriptorSetLayout *descriptor_set_layout =
    xrd_scene_renderer_get_descriptor_set_layout (renderer);

  gchar *model_name = gxr_context_get_device_model_name (context, device_id);
  gboolean is_controller =
  gxr_context_device_is_controller (context, device_id);

  xrd_scene_device_manager_add (self->device_manager, context,
                                device_id, model_name, is_controller,
                                descriptor_set_layout);

  g_free (model_name);
}

static void
_init_device_models (Example *self)
{
  GxrContext *context = self->context;
  for (uint32_t i = GXR_DEVICE_INDEX_HMD + 1; i < GXR_DEVICE_INDEX_MAX; i++)
  {
    if (!gxr_context_is_tracked_device_connected (context, i))
      continue;
    _init_device_model (self, i);
  }
}

static void
_render_eye_cb (uint32_t         eye,
                VkCommandBuffer  cmd_buffer,
                VkPipelineLayout pipeline_layout,
                VkPipeline      *pipelines,
                gpointer         _self)
{
  Example *self = _self;

  graphene_matrix_t vp;
  graphene_matrix_multiply (&self->mat_view[eye],
                            &self->mat_projection[eye], &vp);

  xrd_scene_background_render (self->background, eye,
                               pipelines[PIPELINE_BACKGROUND],
                               pipeline_layout, cmd_buffer, &vp);

  GxrContext *context = self->context;
  xrd_scene_device_manager_render (self->device_manager,
                                   gxr_context_is_input_available (context),
                                   eye, cmd_buffer,
                                   pipelines[PIPELINE_DEVICE_MODELS],
                                   pipeline_layout, &vp);

  GSList *controllers = gxr_context_get_controllers (self->context);
  for (GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = GXR_CONTROLLER (l->data);
      XrdScenePointerTip *scene_tip =
      XRD_SCENE_POINTER_TIP (gxr_controller_get_pointer_tip (controller));
      xrd_scene_pointer_tip_draw (scene_tip, eye,
                                  pipelines[PIPELINE_TIP],
                                  pipeline_layout,
                                  cmd_buffer, &vp);
    }

  xrd_scene_cube_render (self->cube, eye, cmd_buffer, &self->mat_view[eye], &self->mat_projection[eye]);
}

/*
 * Since we are using world space positons for the lights, this only needs
 * to be run once for both eyes
 */
static void
_update_lights_cb (gpointer _self)
{
  Example *self = _self;

  GSList *controllers = gxr_context_get_controllers (self->context);

  XrdSceneRenderer *renderer = xrd_scene_renderer_get_instance ();
  xrd_scene_renderer_update_lights (renderer, controllers);
}

static gboolean
_init_vulkan (Example          *self,
              XrdSceneRenderer *renderer,
              GulkanClient     *gc)
{
  GxrContext *context = self->context;
  if (!xrd_scene_renderer_init_vulkan (renderer, context))
    return FALSE;

  _init_device_models (self);

  GulkanDevice *device = gulkan_client_get_device (gc);
  VkDescriptorSetLayout *descriptor_set_layout =
    xrd_scene_renderer_get_descriptor_set_layout (renderer);

  self->cube = xrd_scene_cube_new ();
  GulkanRenderPass *render_pass = xrd_scene_renderer_get_render_pass (renderer);
  xrd_scene_cube_initialize (self->cube, self->context, GULKAN_RENDERER (renderer), render_pass);


  xrd_scene_background_initialize (self->background,
                                   device,
                                   descriptor_set_layout);

  xrd_scene_renderer_set_render_cb (renderer, _render_eye_cb, self);
  xrd_scene_renderer_set_update_lights_cb (renderer, _update_lights_cb, self);

  return TRUE;
}

static void
_device_activate_cb (void          *context,
                     GxrController *controller,
                     gpointer       _self)
{
  (void) context;
  Example *self = _self;

  g_print ("Controller %lu activated.\n",
           gxr_controller_get_handle (controller));
  XrdScenePointerTip *pointer_tip = xrd_scene_pointer_tip_new ();
  gxr_controller_set_pointer_tip (controller, GXR_POINTER_TIP (pointer_tip));

  /* TODO */
  guint64 device_id = gxr_controller_get_handle (controller);
  _init_device_model (self, (uint32_t) device_id);
}

static void
_device_deactivate_cb (GxrContext    *context,
                       GxrController *controller,
                       gpointer       _self)
{
  (void) context;
  Example *self = _self;
  g_print ("Controller deactivated..\n");

  /* TODO */
  guint64 device_id = gxr_controller_get_handle (controller);
  xrd_scene_device_manager_remove (self->device_manager, (uint32_t) device_id);
}

/* TODO: move to GxrController */
static void
_transform_tip (GxrController *controller,
                graphene_matrix_t *controller_pose,
                GxrContext   *context)
{
  GxrPointerTip *pointer_tip = gxr_controller_get_pointer_tip (controller);


  graphene_point3d_t distance_translation_point;
  graphene_point3d_init (&distance_translation_point,
                         0.f,
                         0.f,
                         -5.0f);

  graphene_matrix_t tip_pose;

  graphene_quaternion_t controller_rotation;
  graphene_quaternion_init_from_matrix (&controller_rotation, controller_pose);

  graphene_point3d_t controller_translation_point;
  graphene_ext_matrix_get_translation_point3d (controller_pose,
                                               &controller_translation_point);

  graphene_matrix_init_identity (&tip_pose);
  graphene_matrix_translate (&tip_pose, &distance_translation_point);
  graphene_matrix_rotate_quaternion (&tip_pose, &controller_rotation);
  graphene_matrix_translate (&tip_pose, &controller_translation_point);

  gxr_pointer_tip_set_transformation (pointer_tip, &tip_pose);

  gxr_pointer_tip_update_apparent_size (pointer_tip, context);

  gxr_pointer_tip_set_active (pointer_tip, FALSE);

  gxr_controller_reset_hover_state (controller);
}

static void
_action_hand_pose_cb (GxrAction    *action,
                      GxrPoseEvent *event,
                      Example      *self)
{
  (void) action;
  if (event->device_connected && event->valid && event->active)
  {
    _transform_tip (event->controller, &event->pose, self->context);
  }
  g_free (event);
}

static void
_action_grab_cb (GxrAction       *action,
                 GxrDigitalEvent *event,
                 Example         *self)
{
  (void) action;
  (void) self;
  g_free (event);
}

static GxrActionSet *
_create_wm_action_set (Example *self)
{
  GxrActionSet *set = gxr_action_set_new_from_url (self->context,
                                                   "/actions/wm");

  gxr_action_set_connect (set, self->context, GXR_ACTION_POSE,
                          "/actions/wm/in/hand_pose",
                          (GCallback) _action_hand_pose_cb, self);
  gxr_action_set_connect (set, self->context, GXR_ACTION_DIGITAL,
                          "/actions/wm/in/grab_window",
                          (GCallback) _action_grab_cb, self);

  return set;
}

static gboolean
_poll_runtime_events (Example *self)
{
  if (!self->context)
    return FALSE;

  gxr_context_poll_event (self->context);

  return TRUE;
}

static gboolean
_poll_input_events (Example *self)
{
  if (!self->context)
  {
    g_printerr ("Error polling events: No Gxr Context\n");
    self->poll_input_source_id = 0;
    return FALSE;
  }

  if (self->actionset_wm == NULL)
    {
      g_printerr ("Error: Action Set not created!\n");
      return TRUE;
    }

  if (!gxr_action_sets_poll (&self->actionset_wm, 1))
    {
      g_printerr ("Error polling actions\n");
      self->poll_input_source_id = 0;
      return FALSE;
    }

  return TRUE;
}

static void
_init_input_callbacks (Example *self)
{
  if (gxr_context_get_api (self->context) == GXR_API_OPENVR)
    {
      if (!gxr_context_load_action_manifest (
        self->context,
        "xrdesktop.openvr",
        "/res/bindings/openvr",
        "actions.json",
        "bindings_vive_controller.json",
        "bindings_knuckles_controller.json",
        NULL))
        {
          g_print ("Failed to load action bindings!\n");
          return;
        }
    }
  else
    {
      if (!gxr_context_load_action_manifest (
        self->context,
        "xrdesktop.openxr",
        "/res/bindings/openxr",
        "actions.json",
        "bindings_khronos_simple_controller.json",
        "bindings_valve_index_controller.json",
        NULL))
        {
          g_print ("Failed to load action bindings!\n");
          return;
        }
    }

  self->actionset_wm = _create_wm_action_set (self);
  gxr_action_sets_attach_bindings (&self->actionset_wm, self->context, 1);

  self->poll_input_source_id =
    g_timeout_add (3, (GSourceFunc) _poll_input_events, self);
}

static void
_system_quit_cb (GxrContext   *context,
                 GxrQuitEvent *event,
                 Example      *self)
{
  (void) event;
  (void) self;
  g_print ("Handling VR quit event %d\n", event->reason);
  gxr_context_acknowledge_quit (context);

  g_main_loop_quit (self->loop);
  g_free (event);
}

static gboolean
_init_example (Example *self)
{
  self->shutdown = false;

  self->poll_runtime_event_source_id = 0;
  self->poll_input_source_id = 0;
  self->actionset_wm = NULL;
  self->context = NULL;
  self->near = 0.05f;
  self->far = 100.0f;
  self->device_manager = xrd_scene_device_manager_new ();
  self->background = xrd_scene_background_new ();

  self->render_source = g_timeout_add (1, _iterate_cb, self);

  g_unix_signal_add (SIGINT, _sigint_cb, self);


  self->context = _create_gxr_context ();
  g_signal_connect (self->context, "device-activate-event",
                    (GCallback) _device_activate_cb, self);
  g_signal_connect (self->context, "device-deactivate-event",
                    (GCallback) _device_deactivate_cb, self);

  GulkanClient *gc = gxr_context_get_gulkan (self->context);
  XrdSceneRenderer *renderer = xrd_scene_renderer_get_instance ();
  if (!_init_vulkan (self, renderer, gc))
    {
      g_object_unref (self);
      g_error ("Could not init Vulkan.\n");
      return FALSE;
    }

  _init_input_callbacks (self);

  self->poll_runtime_event_source_id =
    g_timeout_add (20, (GSourceFunc) _poll_runtime_events, self);

  g_signal_connect (self->context, "quit-event",
                    (GCallback) _system_quit_cb, self);

  return TRUE;
}

int
main (void)
{
  Example self = {
    .loop = g_main_loop_new (NULL, FALSE),
  };

  GxrContext* gxr_context = gxr_context_new_headless ();
  gboolean scene_available =
    !gxr_context_is_another_scene_running (gxr_context);
  g_object_unref (gxr_context);

  if (!scene_available)
    {
      g_error ("Cannot start in scene mode, because another "
                "scene app is already running\n");
      return 1;
    }

  if (!_init_example (&self))
    return 1;

  /* start glib main loop */
  g_main_loop_run (self.loop);

  /* don't clean up when quitting during switching */
  _cleanup (&self);

  g_main_loop_unref (self.loop);

  return 0;
}
