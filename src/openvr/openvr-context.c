/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-context.h"

#include <glib/gprintf.h>
#include "openvr-wrapper.h"

#include <gdk/gdk.h>

#include "gxr-types.h"

#include "openvr-math-private.h"

#include "openvr-context-private.h"
#include "openvr-compositor-private.h"
#include "openvr-system-private.h"

typedef struct _OpenVRContext
{
  GxrContext parent;

  OpenVRFunctions f;

} OpenVROverlayPrivate;

G_DEFINE_TYPE (OpenVRContext, openvr_context, GXR_TYPE_CONTEXT)

#define INIT_FN_TABLE(target, type) \
{ \
  intptr_t ptr = 0; \
  gboolean ret = _init_fn_table (IVR##type##_Version, &ptr); \
  if (!ret || ptr == 0) \
    return false; \
  target = (struct VR_IVR##type##_FnTable*) ptr; \
}

static void
openvr_context_init (OpenVRContext *self)
{
  self->f.system = NULL;
  self->f.overlay = NULL;
  self->f.compositor = NULL;
  self->f.input = NULL;
  self->f.model = NULL;
}

static void
openvr_context_finalize (GObject *gobject)
{
  VR_ShutdownInternal();
  G_OBJECT_CLASS (openvr_context_parent_class)->finalize (gobject);
}

OpenVRContext *
openvr_context_new (void)
{
  return (OpenVRContext*) g_object_new (OPENVR_TYPE_CONTEXT, 0);
}

static gboolean
_init_fn_table (const char *type, intptr_t *ret)
{
  EVRInitError error;
  char fn_table_name[128];
  g_sprintf (fn_table_name, "FnTable:%s", type);

  *ret = VR_GetGenericInterface (fn_table_name, &error);

  if (error != EVRInitError_VRInitError_None)
    {
      g_printerr ("VR_GetGenericInterface returned error %s: %s\n",
                  VR_GetVRInitErrorAsSymbol (error),
                  VR_GetVRInitErrorAsEnglishDescription (error));
      return FALSE;
    }

  return TRUE;
}

static bool
_init_function_tables (OpenVRContext *self)
{
  INIT_FN_TABLE (self->f.system, System)
  INIT_FN_TABLE (self->f.overlay, Overlay)
  INIT_FN_TABLE (self->f.compositor, Compositor)
  INIT_FN_TABLE (self->f.input, Input)
  INIT_FN_TABLE (self->f.model, RenderModels)
  INIT_FN_TABLE (self->f.applications, Applications)
  return true;
}

static gboolean
_vr_init (OpenVRContext *self, EVRApplicationType app_type)
{
  EVRInitError error;
  VR_InitInternal (&error, app_type);

  if (error != EVRInitError_VRInitError_None) {
    g_printerr ("Could not init OpenVR runtime: %s: %s\n",
                VR_GetVRInitErrorAsSymbol (error),
                VR_GetVRInitErrorAsEnglishDescription (error));
    return FALSE;
  }

  if (!_init_function_tables (self))
    {
      g_printerr ("Functions failed to load.\n");
      return FALSE;
    }

  return TRUE;
}

static gboolean
_is_valid (GxrContext *context)
{
  OpenVRContext * self = OPENVR_CONTEXT (context);
  return self->f.system != NULL
    && self->f.overlay != NULL
    && self->f.compositor != NULL
    && self->f.input != NULL
    && self->f.model != NULL;
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
  OpenVRContext * self = OPENVR_CONTEXT (context);

  /* When a(nother) scene app is started, OpenVR sends a
   * SceneApplicationStateChanged event and a quit event, in this case we
   * ignore the quit event and only send an app state changed event.
   */
  gboolean shutdown_event = FALSE;
  gboolean scene_application_state_changed = FALSE;
  GxrQuitReason quit_reason;

  struct VREvent_t vr_event;
  while (self->f.system->PollNextEvent (&vr_event, sizeof (vr_event)))
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

      case EVREventType_VREvent_SceneApplicationStateChanged:
      {
        EVRSceneApplicationState app_state =
          self->f.applications->GetSceneApplicationState ();
        if (app_state == EVRSceneApplicationState_Starting)
          {
            scene_application_state_changed = TRUE;
          }
      } break;

      case EVREventType_VREvent_ProcessQuit:
      {
        scene_application_state_changed = TRUE;
        quit_reason = GXR_QUIT_PROCESS_QUIT;
      } break;

      case EVREventType_VREvent_Quit:
      {
        g_debug ("Event: got quit event, finding out reason...");
        EVRSceneApplicationState app_state =
          self->f.applications->GetSceneApplicationState ();
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
          float latency = self->f.system->GetFloatTrackedDeviceProperty (
            0, vr_event.data.property.prop, NULL);
          g_debug ("Vsync To Photon Latency Prop: %f ms\n", latency * 1000.f);
        }
      } break;

    default:
      g_debug ("Context: Unhandled OpenVR system event: %s\n",
               self->f.system->GetEventTypeNameFromEnum (vr_event.eventType));
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
  OpenVRContext * self = OPENVR_CONTEXT (context);
  self->f.overlay->ShowKeyboard (
    EGamepadTextInputMode_k_EGamepadTextInputModeNormal,
    EGamepadTextInputLineMode_k_EGamepadTextInputLineModeSingleLine,
    "OpenVR System Keyboard", 1, "", TRUE, 0);
}

void
openvr_context_set_system_keyboard_transform (OpenVRContext *self,
                                              graphene_matrix_t *transform)
{
  HmdMatrix34_t openvr_transform;
  openvr_math_graphene_to_matrix34 (transform, &openvr_transform);

  enum ETrackingUniverseOrigin origin = openvr_compositor_get_tracking_space ();
  self->f.overlay->SetKeyboardTransformAbsolute (origin,
                                                &openvr_transform);
}

void
openvr_context_acknowledge_quit (OpenVRContext *self)
{
  self->f.system->AcknowledgeQuit_Exiting ();
}

static gboolean
_init_runtime (GxrContext *context, GxrAppType type);

gboolean
openvr_context_is_another_scene_running (void)
{
  OpenVRContext *ctx = openvr_context_new ();
  _init_runtime (GXR_CONTEXT (ctx), GXR_APP_BACKGROUND);

  /* if applications fntable is not loaded, SteamVR is probably not running. */
  if (ctx->f.applications == NULL)
    return FALSE;

  uint32_t pid = ctx->f.applications->GetCurrentSceneProcessId ();

  g_object_unref (ctx);

  return pid != 0;
}

OpenVRFunctions*
openvr_get_functions (void)
{
  OpenVRContext *context = OPENVR_CONTEXT (gxr_context_get_instance ());
  return &context->f;
}

static void
_get_render_dimensions (GxrContext *context,
                        uint32_t   *width,
                        uint32_t   *height)
{
  (void) context;
  openvr_system_get_render_target_size (width, height);
}

gboolean
_is_input_available ()
{
  return openvr_system_is_input_available ();
}

void
_get_frustum_angles (GxrEye eye,
                     float *left, float *right,
                     float *top, float *bottom)
{
  return openvr_system_get_frustum_angles (eye, left, right, top, bottom);
}

gboolean
_get_head_pose (graphene_matrix_t *pose)
{
  return openvr_system_get_hmd_pose (pose);
}

static gboolean
_init_runtime (GxrContext *context, GxrAppType type)
{
  OpenVRContext *self = OPENVR_CONTEXT (context);
  if (!openvr_context_is_installed ())
  {
    g_printerr ("VR Runtime not installed.\n");
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
    case GXR_APP_BACKGROUND:
      app_type = EVRApplicationType_VRApplication_Background;
      break;
    default:
      app_type = EVRApplicationType_VRApplication_Scene;
      g_warning ("Unknown app type %d\n", type);
  }

  if (!_vr_init (self, app_type))
    return FALSE;

  if (!_is_valid (GXR_CONTEXT (self)))
  {
    g_printerr ("Could not load OpenVR function pointers.\n");
    return FALSE;
  }

  return TRUE;
}

static gboolean
_init_gulkan (GxrContext   *context,
              GulkanClient *gc)
{
  (void) context;
  return openvr_compositor_gulkan_client_init (gc);
}

static gboolean
_init_session (GxrContext   *context,
               GulkanClient *gc)
{
  (void) context;
  (void) gc;
  // openvr does not need any session setup
  return true;
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
  gxr_context_class->is_valid = _is_valid;
  gxr_context_class->init_runtime = _init_runtime;
  gxr_context_class->init_gulkan = _init_gulkan;
  gxr_context_class->init_session = _init_session;
  gxr_context_class->poll_event = _poll_event;
  gxr_context_class->show_keyboard = _show_system_keyboard;
}
