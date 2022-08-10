/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <glib-unix.h>


#include <gxr.h>

#include "scene-pointer-tip.h"
#include "scene-controller.h"

#include "scene-pointer.h"
#include "scene-pointer-tip.h"
#include "scene-renderer.h"
#include "gxr-device-manager.h"
#include "scene-background.h"
#include "scene-cube.h"

typedef struct Example
{
  GMainLoop *loop;
  gboolean restart;

  GxrController *cube_grabbed;
  graphene_matrix_t pointer_pose;

  GxrContext *context;

  guint render_source;
  bool shutdown;
  bool rendering;
  bool framecycle;

  GxrActionSet *actionset;
  guint poll_input_source_id;

  guint poll_runtime_event_source_id;

  gulong device_activate_signal;
  gulong device_deactivate_signal;
  guint sigint_signal;

  graphene_matrix_t mat_view[2];
  graphene_matrix_t mat_projection[2];

  float near;
  float far;

  SceneRenderer *renderer;
  SceneBackground *background;
  gboolean render_background;
  SceneCube *cube;

  // GxrController -> SceneController
  GHashTable *controller_map;
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

  if (self->poll_runtime_event_source_id > 0)
    g_source_remove (self->poll_runtime_event_source_id);
  self->poll_runtime_event_source_id = 0;

  if (self->poll_input_source_id > 0)
    g_source_remove (self->poll_input_source_id);
  self->poll_input_source_id = 0;

  GxrDeviceManager *dm = gxr_context_get_device_manager (self->context);
  g_signal_handler_disconnect(dm, self->device_activate_signal);
  g_signal_handler_disconnect(dm, self->device_deactivate_signal);

  g_source_remove (self->sigint_signal);

  g_clear_object (&self->actionset);

  g_clear_object (&self->background);

  g_clear_object (&self->renderer);

  g_clear_object (&self->cube);

  g_clear_object (&self->context);

  g_hash_table_unref (self->controller_map);

  g_print ("Cleaned up!\n");
}

static gboolean
_iterate_cb (gpointer _self)
{
  Example *self = (Example*) _self;

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
    scene_renderer_draw (self->renderer);

  gxr_context_end_frame (self->context);

  return TRUE;
}

static GxrContext *
_create_gxr_context ()
{
  GSList *instance_ext_list =
    gulkan_context_get_external_memory_instance_extensions ();

  GSList *device_ext_list =
    gulkan_context_get_external_memory_device_extensions ();

  device_ext_list =
    g_slist_append (device_ext_list,
                    g_strdup (VK_KHR_MAINTENANCE1_EXTENSION_NAME));

  GxrContext *context = gxr_context_new_from_vulkan_extensions (instance_ext_list,
                                                                device_ext_list,
                                                                "gxr Demo", 1);

  g_slist_free_full (instance_ext_list, g_free);
  g_slist_free_full (device_ext_list, g_free);
  return context;
}

static void
_render_pointers (Example            *self,
                  VkCommandBuffer     cmd_buffer,
                  VkPipeline         *pipelines,
                  VkPipelineLayout    pipeline_layout,
                  graphene_matrix_t  *vp)
{
  GList *scs = g_hash_table_get_values (self->controller_map);
  for (GList *l = scs; l; l = l->next)
    {
      SceneController *sc = SCENE_CONTROLLER (l->data);

      if (!gxr_controller_is_pointer_pose_valid (scene_controller_get_controller (sc)))
        {
          /*
          g_print ("Pointer pose not valid: %lu\n",
                   scene_controller_get_handle (controller));
          */
          continue;
        }

      ScenePointer *pointer =
        SCENE_POINTER (scene_controller_get_pointer (sc));

      scene_pointer_render (pointer,
                            pipelines[PIPELINE_POINTER],
                            pipeline_layout, cmd_buffer, vp);

      ScenePointerTip *scene_tip =
        SCENE_POINTER_TIP (scene_controller_get_pointer_tip (sc));
      scene_pointer_tip_render (scene_tip,
                                pipelines[PIPELINE_TIP],
                                pipeline_layout,
                                cmd_buffer, vp);
    }
}

static void
_cube_set_position (Example *example)
{
  if (!example->cube_grabbed)
    return;

  graphene_matrix_t pointer_pose;
  gxr_controller_get_pointer_pose (GXR_CONTROLLER (example->cube_grabbed),
                                   &pointer_pose);

  SceneController *sc = g_hash_table_lookup (example->controller_map,
                                             example->cube_grabbed);
  ScenePointer *pointer = scene_controller_get_pointer (sc);
  float distance = scene_pointer_get_default_length (pointer);

  graphene_point3d_t p = { .x = .0f, .y = .0f, .z = - distance };
  graphene_matrix_transform_point3d (&pointer_pose, &p, &p);

  scene_cube_override_position (example->cube, &p);
}

static void
_render_cb (VkCommandBuffer  cmd_buffer,
            VkPipelineLayout pipeline_layout,
            VkPipeline      *pipelines,
            gpointer         _self)
{
  Example *self = _self;

  graphene_matrix_t vp[2];

  for (uint32_t eye = 0; eye < 2; eye++)
    graphene_matrix_multiply (&self->mat_view[eye],
                              &self->mat_projection[eye], &vp[eye]);

  if (self->render_background)
    scene_background_render (self->background,
                             pipelines[PIPELINE_BACKGROUND],
                             pipeline_layout, cmd_buffer,
                             vp);

  _render_pointers (self, cmd_buffer, pipelines, pipeline_layout, vp);

  _cube_set_position (self);
  scene_cube_render (self->cube, cmd_buffer,
                     self->mat_view,
                     self->mat_projection);
}

/*
 * Since we are using world space positons for the lights, this only needs
 * to be run once for both eyes
 */
static void
_update_lights_cb (gpointer _self)
{
  Example *self = _self;

  GList *controllers = g_hash_table_get_values (self->controller_map);
  scene_renderer_update_lights (self->renderer, controllers);
}

static gboolean
_init_vulkan (Example       *self,
              SceneRenderer *renderer,
              GulkanContext  *gc)
{
  GxrContext *context = self->context;
  if (!scene_renderer_init_vulkan (renderer, context))
    return FALSE;

  VkDescriptorSetLayout *descriptor_set_layout =
    scene_renderer_get_descriptor_set_layout (renderer);

  GulkanRenderPass *render_pass = scene_renderer_get_render_pass (renderer);

  VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;

  self->cube = scene_cube_new (gc, GULKAN_RENDERER (renderer),
                               render_pass, sample_count);

  self->background = scene_background_new (gc, descriptor_set_layout);

  scene_renderer_set_render_cb (renderer, _render_cb, self);
  scene_renderer_set_update_lights_cb (renderer, _update_lights_cb, self);

  return TRUE;
}

static void
_device_activate_cb (GxrDeviceManager *dm,
                     GxrController    *controller,
                     gpointer          _self)
{
  (void) dm;

  Example *self = _self;

  g_print ("Controller %lu activated.\n",
           gxr_device_get_handle (GXR_DEVICE (controller)));

  VkDescriptorSetLayout *descriptor_set_layout =
    scene_renderer_get_descriptor_set_layout (self->renderer);
  GulkanContext *client = gxr_context_get_gulkan (self->context);

  VkBuffer lights =
    scene_renderer_get_lights_buffer_handle (self->renderer);

  SceneController *sc = scene_controller_new (controller, self->context);

  ScenePointer *pointer =
    scene_pointer_new (client, descriptor_set_layout);
  scene_controller_set_pointer (sc, pointer);

  ScenePointerTip *pointer_tip =
    scene_pointer_tip_new (client, descriptor_set_layout, lights);
  scene_controller_set_pointer_tip (sc, pointer_tip);

  g_hash_table_insert (self->controller_map, controller, sc);
}

static void
_device_deactivate_cb (GxrDeviceManager *dm,
                       GxrController    *controller,
                       gpointer         _self)
{
  (void) dm;
  Example *self = _self;
  g_print ("Controller deactivated..\n");

  SceneController *sc = g_hash_table_lookup (self->controller_map, controller);
  g_hash_table_remove (self->controller_map, controller);
  g_object_unref (sc);
}

static void
_action_restart_cb (GxrAction       *action,
                    GxrDigitalEvent *event,
                    Example         *self)
{
  (void) action;
  if (event->active && event->changed && event->state)
    {
      g_print ("Restarting example\n");
      self->restart = TRUE;

      g_main_loop_quit (self->loop);
    }
}

static void
_action_quit_cb (GxrAction       *action,
                 GxrDigitalEvent *event,
                 Example         *self)
{
  (void) action;
  if (event->active && event->changed && event->state)
    {
      g_print ("Quitting example\n");
      self->restart = FALSE;

      g_main_loop_quit (self->loop);
    }
}

static void
_action_grab_cb (GxrAction       *action,
                 GxrDigitalEvent *event,
                 Example         *self)
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
_create_wm_action_set (Example *self)
{
  GxrActionSet *set = gxr_action_set_new_from_url (self->context,
                                                   "/actions/wm");


  gxr_action_set_connect (set, self->context, GXR_ACTION_DIGITAL,
                          "/actions/wm/in/show_keyboard",
                          (GCallback) _action_restart_cb, self);

  gxr_action_set_connect_digital_from_float (set, self->context,
                                             "/actions/wm/in/grab_window",
                                             0.25f,
                                             "/actions/wm/out/haptic",
                                             (GCallback) _action_grab_cb,
                                             self);

  gxr_action_set_connect (set, self->context, GXR_ACTION_DIGITAL,
                          "/actions/wm/in/menu",
                          (GCallback) _action_quit_cb, self);


  GxrDeviceManager *dm = gxr_context_get_device_manager (self->context);

  gxr_device_manager_connect_pose_actions (dm, self->context, set,
                                           "/actions/wm/in/hand_pose",
                                           NULL);

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
_init_input_callbacks (Example *self)
{
  if (!gxr_context_load_action_manifest (
    self->context,
    "xrdesktop.openxr",
    "/res/bindings/openxr",
     "actions.json"))
    {
      g_print ("Failed to load action bindings!\n");
      return;
    }

  self->actionset = _create_wm_action_set (self);
  gxr_action_sets_attach_bindings (&self->actionset, self->context, 1);

  self->poll_input_source_id =
    g_timeout_add (3, (GSourceFunc) _poll_input_events, self);
}

static void
_state_change_cb (GxrContext          *context,
                  GxrStateChangeEvent *event,
                  Example             *self)
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
_overlay_cb (GxrContext      *context,
             GxrOverlayEvent *event,
             Example         *self)
{
  (void) context;
  self->render_background = !event->main_session_visible;
}

static gboolean
_init_example (Example *self)
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
  graphene_matrix_init_identity (&self->pointer_pose);

  self->render_source = g_timeout_add (1, _iterate_cb, self);

  self->sigint_signal =
    g_unix_signal_add (SIGINT, _sigint_cb, self);

  self->context = _create_gxr_context ();
  if (!self->context)
    return FALSE;

  self->controller_map = g_hash_table_new (g_direct_hash, g_direct_equal);

  GxrDeviceManager *dm = gxr_context_get_device_manager (self->context);
  self->device_activate_signal =
    g_signal_connect (dm, "device-activate-event",
                      (GCallback) _device_activate_cb, self);
  self->device_deactivate_signal =
    g_signal_connect (dm, "device-deactivate-event",
                      (GCallback) _device_deactivate_cb, self);

  GulkanContext *gc = gxr_context_get_gulkan (self->context);

  self->renderer = scene_renderer_new ();

  if (!_init_vulkan (self, self->renderer, gc))
    {
      g_object_unref (self);
      g_error ("Could not init Vulkan.\n");
      return FALSE;
    }

  _init_input_callbacks (self);

  self->poll_runtime_event_source_id =
    g_timeout_add (20, (GSourceFunc) _poll_runtime_events, self);

  g_signal_connect (self->context, "state-change-event",
                    (GCallback) _state_change_cb, self);

  self->render_background = TRUE;
  g_signal_connect (self->context, "overlay-event",
                    (GCallback) _overlay_cb, self);

  return TRUE;
}

int
main (void)
{
  gboolean restart = FALSE;

  do {
    Example self = {
      .loop = g_main_loop_new (NULL, FALSE),
      .restart = FALSE,
    };

    if (!_init_example (&self))
      return 1;

    /* start glib main loop */
    g_main_loop_run (self.loop);

    restart = self.restart;

    _cleanup (&self);

    g_main_loop_unref (self.loop);
  } while (restart);

  return 0;
}
