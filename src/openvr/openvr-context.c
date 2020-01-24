/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-context.h"

#include "openvr-wrapper.h"

#include <gdk/gdk.h>

#include "gxr-types.h"
#include "gxr-config.h"
#include "gxr-context-private.h"

#include "openvr-math.h"
#include "openvr-overlay.h"
#include "openvr-compositor.h"
#include "openvr-system.h"
#include "openvr-model.h"
#include "openvr-system.h"
#include "openvr-compositor.h"
#include "openvr-action.h"
#include "gxr-io.h"

struct _OpenVRContext
{
  GxrContext parent;

  graphene_matrix_t last_mat_head_pose;
  graphene_matrix_t mat_eye_pos[2];
};

G_DEFINE_TYPE (OpenVRContext, openvr_context, GXR_TYPE_CONTEXT)

/* Functions singleton */
static OpenVRFunctions *functions = NULL;

static void
openvr_context_init (OpenVRContext *self)
{
  graphene_matrix_init_identity (&self->last_mat_head_pose);
}

static void
openvr_context_finalize (GObject *gobject)
{
  VR_ShutdownInternal();
  if (functions)
    g_object_unref (functions);
  G_OBJECT_CLASS (openvr_context_parent_class)->finalize (gobject);
}

OpenVRContext *
openvr_context_new (void)
{
  return (OpenVRContext*) g_object_new (OPENVR_TYPE_CONTEXT, 0);
}

OpenVRFunctions*
openvr_get_functions (void)
{
  if (functions == NULL)
    g_error ("OpenVR function tables were not initialized correctly!\n");
  return functions;
}

static gboolean
_vr_init (EVRApplicationType app_type)
{
  EVRInitError error;
  VR_InitInternal (&error, app_type);

  if (error != EVRInitError_VRInitError_None)
    {
      if (error != EVRInitError_VRInitError_Init_NoServerForBackgroundApp)
        g_error ("Could not init OpenVR runtime: %s: %s\n",
                 VR_GetVRInitErrorAsSymbol (error),
                 VR_GetVRInitErrorAsEnglishDescription (error));

      return FALSE;
    }

  if (functions == NULL || !G_IS_OBJECT (functions))
    {
      functions = openvr_functions_new ();
      if (functions == NULL)
        {
          g_error ("Could not init OpenVR function tables.\n");
          g_object_unref (functions);
          functions = NULL;
          return FALSE;
        }
    }
  else
    {
      g_object_ref (functions);
    }

  return TRUE;
}

gboolean
openvr_context_is_installed (void)
{
  return VR_IsRuntimeInstalled ();
}

gboolean
openvr_context_is_hmd_present (void)
{
  return VR_IsHmdPresent ();
}

static void
_poll_event (GxrContext *context)
{
  /* When a(nother) scene app is started, OpenVR sends a
   * SceneApplicationStateChanged event and a quit event, in this case we
   * ignore the quit event and only send an app state changed event.
   */
  gboolean shutdown_event = FALSE;
  gboolean scene_application_state_changed = FALSE;
  GxrQuitReason quit_reason = GXR_QUIT_SHUTDOWN;
  OpenVRFunctions* f = openvr_get_functions ();

  struct VREvent_t vr_event;
  while (f->system->PollNextEvent (&vr_event, sizeof (vr_event)))
  {
    switch (vr_event.eventType)
    {
      case EVREventType_VREvent_KeyboardCharInput:
      {
        // TODO: https://github.com/ValveSoftware/openvr/issues/289
        char *new_input = g_strdup ((char*) vr_event.data.keyboard.cNewInput);

        g_debug ("Keyboard input %s\n", new_input);
        int len = 0;
        for (; len < 8 && new_input[len] != 0; len++);

        GdkEvent *event = gdk_event_new (GDK_KEY_PRESS);
        event->key.state = TRUE;
        event->key.string = new_input;
        event->key.length = len;
        g_debug ("Event: sending KEYBOARD_PRESS_EVENT signal\n");
        gxr_context_emit_keyboard_press (context, event);
      } break;

      case EVREventType_VREvent_KeyboardClosed:
      {
        g_debug ("Event: sending KEYBOARD_CLOSE_EVENT signal\n");
        gxr_context_emit_keyboard_close (context);
      } break;
#if (OPENVR_VERSION_MINOR >= 8)
      case EVREventType_VREvent_SceneApplicationStateChanged:
      {
        EVRSceneApplicationState app_state =
          f->applications->GetSceneApplicationState ();
        if (app_state == EVRSceneApplicationState_Starting)
          {
            scene_application_state_changed = TRUE;
          }
      } break;
#endif
      case EVREventType_VREvent_ProcessQuit:
      {
        scene_application_state_changed = TRUE;
        quit_reason = GXR_QUIT_PROCESS_QUIT;
      } break;

      case EVREventType_VREvent_Quit:
      {
        g_debug ("Event: got quit event, finding out reason...");
#if (OPENVR_VERSION_MINOR >= 8)
        EVRSceneApplicationState app_state =
          f->applications->GetSceneApplicationState ();
        if (app_state == EVRSceneApplicationState_Quitting)
          {
            g_debug ("Event: Another Scene app is starting");
            scene_application_state_changed = TRUE;
            quit_reason = GXR_QUIT_APPLICATION_TRANSITION;
          }
        else if (app_state == EVRSceneApplicationState_Running)
          {
            g_debug ("Event: SteamVR is quitting");
          }
#endif
        shutdown_event = TRUE;
      } break;

      case EVREventType_VREvent_TrackedDeviceActivated:
      {
        GxrDeviceIndexEvent *event =
          g_malloc (sizeof (GxrDeviceIndexEvent));
        event->controller_handle = vr_event.trackedDeviceIndex;
        g_debug ("Event: sending DEVICE_ACTIVATE_EVENT signal\n");
        gxr_context_emit_device_activate (context, event);
      } break;

      case EVREventType_VREvent_TrackedDeviceDeactivated:
      {
        GxrDeviceIndexEvent *event =
          g_malloc (sizeof (GxrDeviceIndexEvent));
        event->controller_handle = vr_event.trackedDeviceIndex;
        g_debug ("Event: sending DEVICE_DEACTIVATE_EVENT signal\n");
        gxr_context_emit_device_deactivate (context, event);
      } break;

      case EVREventType_VREvent_TrackedDeviceUpdated:
      {
        GxrDeviceIndexEvent *event =
          g_malloc (sizeof (GxrDeviceIndexEvent));
        event->controller_handle = vr_event.trackedDeviceIndex;
        g_debug ("Event: sending DEVICE_UPDATE_EVENT signal\n");
        gxr_context_emit_device_update (context, event);
      } break;

      case EVREventType_VREvent_ActionBindingReloaded:
      {
        g_debug ("Event: sending BINDINGS_UPDATE signal\n");
        gxr_context_emit_bindings_update (context);
      } break;

      case EVREventType_VREvent_Input_ActionManifestReloaded:
      {
        g_debug ("Event: sending ACTIONSET_UPDATE signal\n");
        gxr_context_emit_actionset_update (context);
      } break;

      case EVREventType_VREvent_Input_BindingLoadSuccessful:
      {
        g_debug ("Event: VREvent_Input_BindingLoadSuccessful\n");
        gxr_context_emit_binding_loaded (context);
      } break;

      case EVREventType_VREvent_Input_BindingLoadFailed:
      {
        g_debug ("Event: VREvent_Input_BindingLoadFailed\n");
      } break;

      case EVREventType_VREvent_ProcessConnected:
      {
        g_debug ("Event: VREvent_ProcessConnected\n");
      } break;

      case EVREventType_VREvent_PropertyChanged:
      {
        g_debug ("Event: VREvent_PropertyChanged\n");
        if (vr_event.data.property.prop ==
                ETrackedDeviceProperty_Prop_SecondsFromVsyncToPhotons_Float)
        {
          float latency = f->system->GetFloatTrackedDeviceProperty (
            0, vr_event.data.property.prop, NULL);
          g_debug ("Vsync To Photon Latency Prop: %f ms\n", latency * 1000.f);
        }
      } break;

    default:
      g_debug ("Context: Unhandled OpenVR system event: %s\n",
               f->system->GetEventTypeNameFromEnum (vr_event.eventType));
      break;
    }
  }

  if (shutdown_event && !scene_application_state_changed)
    {
      GxrQuitEvent *event = g_malloc (sizeof (GxrQuitEvent));
      event->reason = GXR_QUIT_SHUTDOWN;
      g_debug ("Event: sending VR_QUIT_SHUTDOWN signal\n");
      gxr_context_emit_quit (context, event);
    }
  else if (scene_application_state_changed)
    {
      GxrQuitEvent *event = g_malloc (sizeof (GxrQuitEvent));
      event->reason = quit_reason;
      gchar *reason =
        (quit_reason == GXR_QUIT_APPLICATION_TRANSITION) ?
        "scene app start" : "scene app stop";

      g_debug ("Event: sending VR_QUIT signal for %s\n", reason);
      gxr_context_emit_quit (context, event);
    }
}

static void
_show_system_keyboard (GxrContext *context)
{
  (void) context;
  OpenVRFunctions* f = openvr_get_functions ();
  f->overlay->ShowKeyboard (
    EGamepadTextInputMode_k_EGamepadTextInputModeNormal,
    EGamepadTextInputLineMode_k_EGamepadTextInputLineModeSingleLine,
    "OpenVR System Keyboard", 1, "", TRUE, 0);
}

static void
_set_keyboard_transform (GxrContext        *context,
                         graphene_matrix_t *transform)
{
  (void) context;
  HmdMatrix34_t openvr_transform;
  openvr_math_graphene_to_matrix34 (transform, &openvr_transform);

  OpenVRFunctions *f = openvr_get_functions();
  enum ETrackingUniverseOrigin origin = f->compositor->GetTrackingSpace ();
  f->overlay->SetKeyboardTransformAbsolute (origin,
                                            &openvr_transform);
}

static void
_acknowledge_quit (GxrContext *context)
{
  (void) context;
  OpenVRFunctions *f = openvr_get_functions();
  f->system->AcknowledgeQuit_Exiting ();
}

static gboolean
_is_another_scene_running (GxrContext *context)
{
  (void) context;
  OpenVRFunctions *f = openvr_get_functions();
  /* if applications fntable is not loaded, SteamVR is probably not running. */
  if (f->applications == NULL)
    return FALSE;

  uint32_t pid = f->applications->GetCurrentSceneProcessId ();
  return pid != 0;
}

static void
_get_render_dimensions (GxrContext *context,
                        uint32_t   *width,
                        uint32_t   *height)
{
  (void) context;
  openvr_system_get_render_target_size (width, height);
}

static gboolean
_is_input_available ()
{
  return openvr_system_is_input_available ();
}

static void
_get_frustum_angles (GxrContext *context, GxrEye eye,
                     float *left, float *right,
                     float *top, float *bottom)
{
  (void) context;
  openvr_system_get_frustum_angles (eye, left, right, top, bottom);
}

static gboolean
_get_head_pose (GxrContext *context, graphene_matrix_t *pose)
{
  (void) context;
  return openvr_system_get_hmd_pose (pose);
}

static gboolean
_init_runtime (GxrContext *context, GxrAppType type)
{
  OpenVRContext *self = OPENVR_CONTEXT (context);
  if (!openvr_context_is_installed ())
  {
    g_error ("VR Runtime not installed.\n");
    return FALSE;
  }

  EVRApplicationType app_type;

  switch (type)
  {
    case GXR_APP_SCENE:
      app_type = EVRApplicationType_VRApplication_Scene;
      break;
    case GXR_APP_OVERLAY:
      app_type = EVRApplicationType_VRApplication_Overlay;
      break;
    case GXR_APP_HEADLESS:
      app_type = EVRApplicationType_VRApplication_Background;
      break;
    default:
      app_type = EVRApplicationType_VRApplication_Scene;
      g_warning ("Unknown app type %d\n", type);
  }

  if (!_vr_init (app_type))
    return FALSE;

  if (!openvr_functions_is_valid (functions))
  {
    g_error ("Could not load OpenVR function pointers.\n");
    return FALSE;
  }

  if (type != GXR_APP_HEADLESS)
    for (uint32_t eye = 0; eye < 2; eye++)
      {
        self->mat_eye_pos[eye] = openvr_system_get_eye_to_head_transform (eye);
        graphene_matrix_inverse (&self->mat_eye_pos[eye],
                                 &self->mat_eye_pos[eye]);
      }

  return TRUE;
}

static gboolean
_init_session (GxrContext   *context)
{
  (void) context;
  // openvr does not need any session setup
  return true;
}

static gboolean
_init_framebuffers (GxrContext           *context,
                    VkExtent2D            extent,
                    VkSampleCountFlagBits sample_count,
                    GulkanFrameBuffer    *framebuffers[2],
                    GulkanRenderPass    **render_pass)
{
  GulkanClient *gc = gxr_context_get_gulkan (context);
  GulkanDevice *device = gulkan_client_get_device (gc);

  *render_pass =
    gulkan_render_pass_new (device, sample_count,
                            VK_FORMAT_R8G8B8A8_UNORM,
                            VK_IMAGE_LAYOUT_UNDEFINED, TRUE);
  if (!*render_pass)
    {
      g_printerr ("Could not init render pass.\n");
      return FALSE;
    }

  for (uint32_t eye = 0; eye < 2; eye++)
    {
      framebuffers[eye] = gulkan_frame_buffer_new (device, *render_pass,
                                                   extent, sample_count,
                                                   VK_FORMAT_R8G8B8A8_UNORM,
                                                   TRUE);
      if (!framebuffers[eye])
        return FALSE;
    }

  return true;
}

static gboolean
_submit_framebuffers (GxrContext           *context,
                      GulkanFrameBuffer    *framebuffers[2],
                      VkExtent2D            extent,
                      VkSampleCountFlagBits sample_count)
{
  GulkanClient *gc = gxr_context_get_gulkan (context);

  VkImage left =
    gulkan_frame_buffer_get_color_image (framebuffers[GXR_EYE_LEFT]);

  VkImage right =
    gulkan_frame_buffer_get_color_image (framebuffers[GXR_EYE_RIGHT]);

  if (!openvr_compositor_submit (gc, extent.width, extent.height,
                                 VK_FORMAT_R8G8B8A8_UNORM,
                                 sample_count, left, right))
    return FALSE;

  return TRUE;
}

static uint32_t
_get_model_vertex_stride (GxrContext *self)
{
  (void) self;
  return openvr_model_get_vertex_stride ();
}

static uint32_t
_get_model_normal_offset (GxrContext *self)
{
  (void) self;
  return openvr_model_get_normal_offset ();
}

static uint32_t
_get_model_uv_offset (GxrContext *self)
{
  (void) self;
  return openvr_model_get_uv_offset ();
}

static void
_get_projection (GxrContext *context,
                 GxrEye eye,
                 float near,
                 float far,
                 graphene_matrix_t *mat)
{
  (void) context;
  *mat = openvr_system_get_projection_matrix (eye, near, far);
}

static void
_get_view (GxrContext *context,
           GxrEye eye,
           graphene_matrix_t *mat)
{
  OpenVRContext *self = OPENVR_CONTEXT (context);
  graphene_matrix_multiply (&self->last_mat_head_pose,
                            &self->mat_eye_pos[eye], mat);
}

static gboolean
_begin_frame (GxrContext *context)
{
  (void) context;
  return TRUE;
}

static gboolean
_end_frame (GxrContext *context,
            GxrPose *poses)
{
  OpenVRContext *self = OPENVR_CONTEXT (context);
  openvr_compositor_wait_get_poses (poses, GXR_DEVICE_INDEX_MAX);

  if (poses[GXR_DEVICE_INDEX_HMD].is_valid)
    graphene_matrix_inverse (&poses[GXR_DEVICE_INDEX_HMD].transformation,
                             &self->last_mat_head_pose);

  return TRUE;
}

static gboolean
_is_tracked_device_connected (GxrContext *context, uint32_t i)
{
  (void) context;
  return openvr_system_is_tracked_device_connected (i);
}

static gboolean
_device_is_controller (GxrContext *context, uint32_t i)
{
  (void) context;
  return openvr_system_device_is_controller (i);
}

static gchar*
_get_device_model_name (GxrContext *context, uint32_t i)
{
  (void) context;
  return openvr_system_get_device_model_name (i);
}

static gboolean
_load_model (GxrContext         *context,
             GulkanVertexBuffer *vbo,
             GulkanTexture     **texture,
             VkSampler          *sampler,
             const char         *model_name)
{
  GulkanClient *gc = gxr_context_get_gulkan (context);
  return openvr_model_load (gc, vbo, texture, sampler, model_name);
}

static GSList *
_get_model_list (GxrContext *self)
{
  (void) self;
  return openvr_model_get_list ();
}

static GxrActionSet *
_new_action_set_from_url (GxrContext *context, gchar *url)
{
  return (GxrActionSet*) openvr_action_set_new_from_url (OPENVR_CONTEXT(context), url);
}

static gboolean
_load_manifest (char *path)
{
  OpenVRFunctions *f = openvr_get_functions ();

  g_print ("Load manifest path %s\n", path);

  EVRInputError err;
  err = f->input->SetActionManifestPath (path);

  if (err != EVRInputError_VRInputError_None)
  {
    g_printerr ("ERROR: SetActionManifestPath: %s\n",
                openvr_input_error_string (err));
    return FALSE;
  }
  return TRUE;
}

static gboolean
_load_action_manifest (GxrContext *self,
                       const char *cache_name,
                       const char *resource_path,
                       const char *manifest_name,
                       const char *first_binding,
                       va_list     args)
{
  (void) self;
  /* Create cache directory if needed */
  GString* cache_path = gxr_io_get_cache_path (cache_name);

  if (g_mkdir_with_parents (cache_path->str, 0700) == -1)
    {
      g_printerr ("Unable to create directory %s\n", cache_path->str);
      return FALSE;
    }

  /* Cache actions manifest */
  GString *actions_path = g_string_new ("");
  if (!gxr_io_write_resource_to_file (resource_path,
    cache_path->str,
    manifest_name,
    actions_path))
    return FALSE;

  const char* current = first_binding;
  while (current != NULL)
    {
      GString *bindings_path = g_string_new ("");
      if (!gxr_io_write_resource_to_file (resource_path,
                                          cache_path->str,
                                          current,
                                          bindings_path))
        return FALSE;

      g_string_free (bindings_path, TRUE);

      current = va_arg (args, const char*);
    }

  g_string_free (cache_path, TRUE);

  if (!_load_manifest (actions_path->str))
    return FALSE;

  g_string_free (actions_path, TRUE);

  return TRUE;
}

static GxrAction *
_new_action_from_type_url (GxrContext   *self,
                           GxrActionSet *action_set,
                           GxrActionType type,
                           char          *url)
{
  (void) self;
  return GXR_ACTION (openvr_action_new_from_type_url (action_set, type, url));
}

static GxrOverlay *
_new_overlay (GxrContext *self, gchar* key)
{
  (void) self;
  return GXR_OVERLAY (openvr_overlay_new (key));
}

static void
_request_quit (GxrContext *context)
{
  GxrQuitEvent *event = g_malloc (sizeof (GxrQuitEvent));
  event->reason = GXR_QUIT_SHUTDOWN;
  g_debug ("Event: sending VR_QUIT_SHUTDOWN signal\n");
  gxr_context_emit_quit (context, event);
}

static void
_split (gchar *str, GSList **out_list)
{
  gchar **array = g_strsplit (str, " ", 0);
  int i = 0;
  while(array[i] != NULL)
    {
      *out_list = g_slist_append (*out_list, g_strdup (array[i]));
      i++;
    }
  g_strfreev (array);
}


static bool
_get_instance_extensions (GxrContext *self, GSList **out_list)
{
  (void) self;
  OpenVRFunctions *f = openvr_get_functions ();

  uint32_t size =
    f->compositor->GetVulkanInstanceExtensionsRequired (NULL, 0);

  if (size > 0)
    {
      gchar *extensions = g_malloc (sizeof(gchar) * size);
      extensions[0] = 0;
      f->compositor->GetVulkanInstanceExtensionsRequired (extensions, size);
      _split (extensions, out_list);
      g_free(extensions);
    }

  return TRUE;
}

static bool
_get_device_extensions (GxrContext   *self,
                        GulkanClient *gc,
                        GSList      **out_list)
{
  (void) self;
  OpenVRFunctions *f = openvr_get_functions ();

  VkPhysicalDevice physical_device = 0;
  f->system->GetOutputDevice ((uint64_t *) &physical_device,
                              ETextureType_TextureType_Vulkan,
                              (struct VkInstance_T *)
                              gulkan_client_get_instance_handle (gc));


  uint32_t size = f->compositor->
    GetVulkanDeviceExtensionsRequired (physical_device, NULL, 0);

  if (size > 0)
    {
      gchar *extensions = g_malloc (sizeof(gchar) * size);
      extensions[0] = 0;
      f->compositor->GetVulkanDeviceExtensionsRequired (
        physical_device, extensions, size);

      _split (extensions, out_list);
      g_free (extensions);
    }

  return TRUE;
}

static void
openvr_context_class_init (OpenVRContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = openvr_context_finalize;

  GxrContextClass *gxr_context_class = GXR_CONTEXT_CLASS (klass);
  gxr_context_class->get_render_dimensions = _get_render_dimensions;
  gxr_context_class->is_input_available = _is_input_available;
  gxr_context_class->get_frustum_angles = _get_frustum_angles;
  gxr_context_class->get_head_pose = _get_head_pose;
  gxr_context_class->init_runtime = _init_runtime;
  gxr_context_class->init_session = _init_session;
  gxr_context_class->poll_event = _poll_event;
  gxr_context_class->show_keyboard = _show_system_keyboard;
  gxr_context_class->init_framebuffers = _init_framebuffers;
  gxr_context_class->get_model_vertex_stride = _get_model_vertex_stride;
  gxr_context_class->get_model_normal_offset = _get_model_normal_offset;
  gxr_context_class->get_model_uv_offset = _get_model_uv_offset;
  gxr_context_class->submit_framebuffers = _submit_framebuffers;
  gxr_context_class->get_projection = _get_projection;
  gxr_context_class->get_view = _get_view;
  gxr_context_class->begin_frame = _begin_frame;
  gxr_context_class->end_frame = _end_frame;
  gxr_context_class->acknowledge_quit = _acknowledge_quit;
  gxr_context_class->is_tracked_device_connected = _is_tracked_device_connected;
  gxr_context_class->device_is_controller = _device_is_controller;
  gxr_context_class->get_device_model_name = _get_device_model_name;
  gxr_context_class->load_model = _load_model;
  gxr_context_class->is_another_scene_running = _is_another_scene_running;
  gxr_context_class->set_keyboard_transform = _set_keyboard_transform;
  gxr_context_class->get_model_list = _get_model_list;
  gxr_context_class->new_action_set_from_url = _new_action_set_from_url;
  gxr_context_class->load_action_manifest = _load_action_manifest;
  gxr_context_class->new_action_from_type_url = _new_action_from_type_url;
  gxr_context_class->new_overlay = _new_overlay;
  gxr_context_class->request_quit = _request_quit;
  gxr_context_class->get_instance_extensions = _get_instance_extensions;
  gxr_context_class->get_device_extensions = _get_device_extensions;
}
