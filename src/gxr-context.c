/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-context.h"
#include "gxr-config.h"
#include "gxr-backend.h"

typedef struct _GxrContextPrivate
{
  GObject parent;
  GxrApi api;
} GxrContextPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GxrContext, gxr_context, G_TYPE_OBJECT)

enum {
  KEYBOARD_PRESS_EVENT,
  KEYBOARD_CLOSE_EVENT,
  QUIT_EVENT,
  DEVICE_ACTIVATE_EVENT,
  DEVICE_DEACTIVATE_EVENT,
  DEVICE_UPDATE_EVENT,
  BINDING_LOADED,
  BINDINGS_UPDATE,
  ACTIONSET_UPDATE,
  LAST_SIGNAL
};

static guint context_signals[LAST_SIGNAL] = { 0 };

static void
gxr_context_finalize (GObject *gobject);

static void
gxr_context_class_init (GxrContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gxr_context_finalize;

  context_signals[KEYBOARD_PRESS_EVENT] =
  g_signal_new ("keyboard-press-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL, G_TYPE_NONE,
                1, G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE);

  context_signals[KEYBOARD_CLOSE_EVENT] =
  g_signal_new ("keyboard-close-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  context_signals[QUIT_EVENT] =
  g_signal_new ("quit-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL, G_TYPE_NONE,
                1, G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE);

  context_signals[DEVICE_ACTIVATE_EVENT] =
  g_signal_new ("device-activate-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL, G_TYPE_NONE,
                1, G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE);

  context_signals[DEVICE_DEACTIVATE_EVENT] =
  g_signal_new ("device-deactivate-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL, G_TYPE_NONE,
                1, G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE);

  context_signals[DEVICE_UPDATE_EVENT] =
  g_signal_new ("device-update-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL, G_TYPE_NONE,
                1, G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE);

  context_signals[BINDINGS_UPDATE] =
  g_signal_new ("bindings-update-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  context_signals[BINDING_LOADED] =
  g_signal_new ("binding-loaded-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  context_signals[ACTIONSET_UPDATE] =
  g_signal_new ("action-set-update-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static GxrApi
_get_api_from_env ()
{
  const gchar *api_env = g_getenv ("GXR_API");
  if (g_strcmp0 (api_env, "openxr") == 0)
    return GXR_API_OPENXR;
  else if (g_strcmp0 (api_env, "openvr") == 0)
    return GXR_API_OPENVR;
  else
    return GXR_DEFAULT_API;
}

GxrContext *
gxr_context_new_from_api (GxrApi api)
{
  return gxr_backend_new_context (gxr_backend_get_instance (api));
}

static GxrContext*
_new_context_from_env ()
{
  return gxr_context_new_from_api (_get_api_from_env ());
}

static void
gxr_context_init (GxrContext *self)
{
  GxrContextPrivate *priv = gxr_context_get_instance_private (self);
  priv->api = GXR_API_NONE;
}

GxrContext *
gxr_context_new ()
{
  return _new_context_from_env ();
}

static void
gxr_context_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (gxr_context_parent_class)->finalize (gobject);
}

GxrApi
gxr_context_get_api (GxrContext *self)
{
  GxrContextPrivate *priv = gxr_context_get_instance_private (self);
  return priv->api;
}


void
gxr_context_set_api (GxrContext *self, GxrApi api)
{
  GxrContextPrivate *priv = gxr_context_get_instance_private (self);
  priv->api = api;
}

gboolean
gxr_context_get_head_pose (GxrContext *self, graphene_matrix_t *pose)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_head_pose == NULL)
    return FALSE;
  return klass->get_head_pose (self, pose);
}

void
gxr_context_get_frustum_angles (GxrContext *self, GxrEye eye,
                                float *left, float *right,
                                float *top, float *bottom)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_frustum_angles == NULL)
    return;
  klass->get_frustum_angles (self, eye, left, right, top, bottom);
}

gboolean
gxr_context_is_input_available (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->is_input_available == NULL)
    return FALSE;
  return klass->is_input_available (self);
}

void
gxr_context_get_render_dimensions (GxrContext *self,
                                   uint32_t *width,
                                   uint32_t *height)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_render_dimensions == NULL)
      return;
  klass->get_render_dimensions (self, width, height);
}

gboolean
gxr_context_is_valid (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->is_valid == NULL)
      return FALSE;
  return klass->is_valid (self);
}

gboolean
gxr_context_init_runtime (GxrContext *self, GxrAppType type)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->init_runtime == NULL)
    return FALSE;
  return klass->init_runtime (self, type);
}

gboolean
gxr_context_init_gulkan (GxrContext *self, GulkanClient *gc)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->init_gulkan == NULL)
      return FALSE;
  return klass->init_gulkan (self, gc);
}

gboolean
gxr_context_init_session (GxrContext *self, GulkanClient *gc)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->init_session == NULL)
    return FALSE;
  return klass->init_session (self, gc);
}

void
gxr_context_poll_event (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->poll_event == NULL)
    return;
  klass->poll_event (self);
}

void
gxr_context_show_keyboard (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->show_keyboard == NULL)
    return;
  klass->show_keyboard (self);
}

void
gxr_context_emit_keyboard_press (GxrContext *self, gpointer event)
{
  g_signal_emit (self, context_signals[KEYBOARD_PRESS_EVENT], 0, event);
}

void
gxr_context_emit_keyboard_close (GxrContext *self)
{
  g_signal_emit (self, context_signals[KEYBOARD_CLOSE_EVENT], 0);
}

void
gxr_context_emit_quit (GxrContext *self, gpointer event)
{
  g_signal_emit (self, context_signals[QUIT_EVENT], 0, event);
}

void
gxr_context_emit_device_activate (GxrContext *self, gpointer event)
{
  g_signal_emit (self, context_signals[DEVICE_ACTIVATE_EVENT], 0, event);
}

void
gxr_context_emit_device_deactivate (GxrContext *self, gpointer event)
{
  g_signal_emit (self, context_signals[DEVICE_DEACTIVATE_EVENT], 0, event);
}

void
gxr_context_emit_device_update (GxrContext *self, gpointer event)
{
  g_signal_emit (self, context_signals[DEVICE_UPDATE_EVENT], 0, event);
}

void
gxr_context_emit_bindings_update (GxrContext *self)
{
  g_signal_emit (self, context_signals[BINDINGS_UPDATE], 0);
}

void
gxr_context_emit_binding_loaded (GxrContext *self)
{
  g_signal_emit (self, context_signals[BINDING_LOADED], 0);
}

void
gxr_context_emit_actionset_update (GxrContext *self)
{
  g_signal_emit (self, context_signals[ACTIONSET_UPDATE], 0);
}

gboolean
gxr_context_init_framebuffers (GxrContext           *self,
                               GulkanFrameBuffer    *framebuffers[2],
                               GulkanClient         *gc,
                               uint32_t              width,
                               uint32_t              height,
                               VkSampleCountFlagBits msaa_sample_count)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->init_framebuffers == NULL)
    return FALSE;
  return klass->init_framebuffers (self, framebuffers, gc,
                                   width, height, msaa_sample_count);
}

gboolean
gxr_context_submit_framebuffers (GxrContext           *self,
                                 GulkanFrameBuffer    *framebuffers[2],
                                 GulkanClient         *gc,
                                 uint32_t              width,
                                 uint32_t              height,
                                 VkSampleCountFlagBits msaa_sample_count)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->submit_framebuffers == NULL)
    return FALSE;
  return klass->submit_framebuffers (self, framebuffers, gc,
                                     width, height, msaa_sample_count);
}

uint32_t
gxr_context_get_model_vertex_stride (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_model_vertex_stride == NULL)
    return 0;
  return klass->get_model_vertex_stride (self);
}

uint32_t
gxr_context_get_model_normal_offset (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_model_normal_offset == NULL)
    return 0;
  return klass->get_model_normal_offset (self);
}

uint32_t
gxr_context_get_model_uv_offset (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_model_uv_offset == NULL)
    return 0;
  return klass->get_model_uv_offset (self);
}

void
gxr_context_get_projection (GxrContext *self,
                            GxrEye eye,
                            float near,
                            float far,
                            graphene_matrix_t *mat)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_projection == NULL)
    return;
  klass->get_projection (self, eye, near, far, mat);
}

void
gxr_context_get_view (GxrContext *self,
                      GxrEye eye,
                      graphene_matrix_t *mat)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_view == NULL)
    return;
  klass->get_view (self, eye, mat);
}

gboolean
gxr_context_begin_frame (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->begin_frame == NULL)
    return FALSE;
  return klass->begin_frame (self);
}

gboolean
gxr_context_end_frame (GxrContext *self,
                       GxrPose *poses)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->end_frame == NULL)
    return FALSE;
  return klass->end_frame (self, poses);
}

GxrActionSet *
gxr_context_new_action_set_from_url (GxrContext *self, gchar *url)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->new_action_set_from_url == NULL)
    return FALSE;
  return klass->new_action_set_from_url (self, url);
}

gboolean
gxr_context_load_action_manifest (GxrContext *self,
                                  const char *cache_name,
                                  const char *resource_path,
                                  const char *manifest_name,
                                  const char *first_binding,
                                  ...)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->load_action_manifest == NULL)
    return FALSE;

  va_list args;
  va_start (args, first_binding);

  gboolean ret = klass->load_action_manifest (self,
                                              cache_name,
                                              resource_path,
                                              manifest_name,
                                              first_binding,
                                              args);

  va_end (args);
  return ret;
}

void
gxr_context_acknowledge_quit (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->acknowledge_quit == NULL)
    return;
  klass->acknowledge_quit (self);
}

gboolean
gxr_context_is_tracked_device_connected (GxrContext *self, uint32_t i)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->is_tracked_device_connected == NULL)
    return FALSE;
  return klass->is_tracked_device_connected (self, i);
}

gboolean
gxr_context_device_is_controller (GxrContext *self, uint32_t i)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->device_is_controller == NULL)
    return FALSE;
  return klass->device_is_controller (self, i);
}

gchar*
gxr_context_get_device_model_name (GxrContext *self, uint32_t i)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_device_model_name == NULL)
    return (gchar*) g_malloc (1);
  return klass->get_device_model_name (self, i);
}

gboolean
gxr_context_load_model (GxrContext         *self,
                        GulkanClient       *gc,
                        GulkanVertexBuffer *vbo,
                        GulkanTexture     **texture,
                        VkSampler          *sampler,
                        const char         *model_name)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->load_model == NULL)
    return FALSE;
  return klass->load_model (self, gc, vbo, texture, sampler, model_name);
}

gboolean
gxr_context_is_another_scene_running (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->is_another_scene_running  == NULL)
    return FALSE;
  return klass->is_another_scene_running (self);
}

gboolean
gxr_context_inititalize (GxrContext   *self,
                         GulkanClient *gc,
                         GxrAppType    type)
{
  if (!gxr_context_init_runtime (self, type))
    {
      g_printerr ("Could not init VR runtime.\n");
      return FALSE;
    }

  if (!gxr_context_init_gulkan (self, gc))
    {
      g_printerr ("Could not initialize Gulkan!\n");
      return FALSE;
    }

  if (!gxr_context_init_session (self, gc))
    {
      g_printerr ("Could not init VR session.\n");
      return FALSE;
    }

  return TRUE;
}

void
gxr_context_request_quit (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->request_quit  == NULL)
    return;
  klass->request_quit (self);
}

void
gxr_context_set_keyboard_transform (GxrContext        *self,
                                    graphene_matrix_t *transform)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->set_keyboard_transform  == NULL)
    return;
  klass->set_keyboard_transform (self, transform);
}

GxrAction *
gxr_context_new_action_from_type_url (GxrContext   *self,
                                      GxrActionSet *action_set,
                                      GxrActionType type,
                                      char          *url)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->new_action_from_type_url == NULL)
    return FALSE;
  return klass->new_action_from_type_url (self, action_set, type, url);
}

GxrOverlay *
gxr_context_new_overlay (GxrContext *self, gchar* key)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->new_overlay == NULL)
    return FALSE;
  return klass->new_overlay (self, key);
}

GSList *
gxr_context_get_model_list (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_model_list == NULL)
    return FALSE;
  return klass->get_model_list (self);
}

