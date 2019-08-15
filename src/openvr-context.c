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

#include "gxr-math.h"

#include "openvr-context-private.h"

typedef struct _OpenVRContext
{
  GObject parent;

  OpenVRFunctions f;

  enum ETrackingUniverseOrigin origin;
} OpenVROverlayPrivate;

G_DEFINE_TYPE (OpenVRContext, openvr_context, G_TYPE_OBJECT)

#define INIT_FN_TABLE(target, type) \
{ \
  intptr_t ptr = 0; \
  gboolean ret = _init_fn_table (IVR##type##_Version, &ptr); \
  if (!ret || ptr == 0) \
    return false; \
  target = (struct VR_IVR##type##_FnTable*) ptr; \
}

enum {
  KEYBOARD_PRESS_EVENT,
  KEYBOARD_CLOSE_EVENT,
  QUIT_EVENT,
  DEVICE_ACTIVATE_EVENT,
  DEVICE_DEACTIVATE_EVENT,
  DEVICE_UPDATE_EVENT,
  LAST_SIGNAL
};

static guint context_signals[LAST_SIGNAL] = { 0 };

// singleton variable that can be set to NULL again when finalizing the context
static OpenVRContext *context = NULL;

static void
openvr_context_finalize (GObject *gobject);

static void
openvr_context_class_init (OpenVRContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = openvr_context_finalize;

  context_signals[KEYBOARD_PRESS_EVENT] =
    g_signal_new ("keyboard-press-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

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
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  context_signals[DEVICE_ACTIVATE_EVENT] =
    g_signal_new ("device-activate-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  context_signals[DEVICE_DEACTIVATE_EVENT] =
    g_signal_new ("device-deactivate-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  context_signals[DEVICE_UPDATE_EVENT] =
    g_signal_new ("device-update-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
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

  context = NULL;

  G_OBJECT_CLASS (openvr_context_parent_class)->finalize (gobject);
}

static OpenVRContext *
openvr_context_new (void)
{
  return (OpenVRContext*) g_object_new (OPENVR_TYPE_CONTEXT, 0);
}

OpenVRContext *
openvr_context_get_instance ()
{
  if (context == NULL)
    context = openvr_context_new ();

  return context;
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

  self->origin = self->f.compositor->GetTrackingSpace ();

  return TRUE;
}

gboolean
openvr_context_initialize (OpenVRContext *self, OpenVRAppType type)
{
  if (!openvr_context_is_installed ())
    {
      g_printerr ("VR Runtime not installed.\n");
      return FALSE;
    }

  EVRApplicationType app_type;

  switch (type)
    {
      case OPENVR_APP_SCENE:
        app_type = EVRApplicationType_VRApplication_Scene;
        break;
      case OPENVR_APP_OVERLAY:
        app_type = EVRApplicationType_VRApplication_Overlay;
        break;
      case OPENVR_APP_BACKGROUND:
        app_type = EVRApplicationType_VRApplication_Background;
        break;
      default:
        app_type = EVRApplicationType_VRApplication_Scene;
        g_warning ("Unknown app type %d\n", type);
    }

  if (!_vr_init (self, app_type))
    return FALSE;

  if (!openvr_context_is_valid (self))
    {
      g_printerr ("Could not load OpenVR function pointers.\n");
      return FALSE;
    }

  return TRUE;
}

gboolean
openvr_context_is_valid (OpenVRContext * self)
{
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

void
openvr_context_poll_event (OpenVRContext *self)
{
  /* Starting another VR app will first emit a shutdown event, and then the
   * app transition event. In gxr, a shutdown event is only emitted
   * when the app should completely quit.
   * If another VR app is started, only the app transition event is emitted. */
  gboolean shutdown_event_pending = FALSE;

  struct VREvent_t vr_event;
  while (self->f.system->PollNextEvent (&vr_event, sizeof (vr_event)))
  {
    switch (vr_event.eventType)
    {
      case EVREventType_VREvent_KeyboardCharInput:
      {
        // TODO: https://github.com/ValveSoftware/openvr/issues/289
        char *new_input = (char*) vr_event.data.keyboard.cNewInput;

        g_print ("Keyboard input %s\n", new_input);
        int len = 0;
        for (; len < 8 && new_input[len] != 0; len++);

        GdkEvent *event = gdk_event_new (GDK_KEY_PRESS);
        event->key.state = TRUE;
        event->key.string = new_input;
        event->key.length = len;
        g_signal_emit (self, context_signals[KEYBOARD_PRESS_EVENT], 0, event);
      } break;

      case EVREventType_VREvent_KeyboardClosed:
      {
        g_signal_emit (self, context_signals[KEYBOARD_CLOSE_EVENT], 0);
      } break;

      case EVREventType_VREvent_ApplicationTransitionStarted:
      {
        shutdown_event_pending = FALSE;
        OpenVRQuitEvent *event = g_malloc (sizeof (OpenVRQuitEvent));
        event->reason = VR_QUIT_APPLICATION_TRANSITION;
        g_signal_emit (self, context_signals[QUIT_EVENT], 0, event);
      } break;

      case EVREventType_VREvent_ProcessQuit:
      {
        shutdown_event_pending = FALSE;
        OpenVRQuitEvent *event = g_malloc (sizeof (OpenVRQuitEvent));
        event->reason = VR_QUIT_PROCESS_QUIT;
        g_signal_emit (self, context_signals[QUIT_EVENT], 0, event);
      } break;

      case EVREventType_VREvent_Quit:
      {
        shutdown_event_pending = TRUE;
      } break;

      case EVREventType_VREvent_TrackedDeviceActivated:
      {
        OpenVRDeviceIndexEvent *event =
          g_malloc (sizeof (OpenVRDeviceIndexEvent));
        event->controller_handle = vr_event.trackedDeviceIndex;
        g_signal_emit (self, context_signals[DEVICE_ACTIVATE_EVENT], 0, event);
      } break;

      case EVREventType_VREvent_TrackedDeviceDeactivated:
      {
        OpenVRDeviceIndexEvent *event =
          g_malloc (sizeof (OpenVRDeviceIndexEvent));
        event->controller_handle = vr_event.trackedDeviceIndex;
        g_signal_emit (self, context_signals[DEVICE_DEACTIVATE_EVENT], 0, event);
      } break;

      case EVREventType_VREvent_TrackedDeviceUpdated:
      {
        OpenVRDeviceIndexEvent *event =
          g_malloc (sizeof (OpenVRDeviceIndexEvent));
        event->controller_handle = vr_event.trackedDeviceIndex;
        g_signal_emit (self, context_signals[DEVICE_UPDATE_EVENT], 0, event);
      } break;

    default:
      //g_print ("Context: Unhandled OpenVR system event: %s\n",
      //         self->system->GetEventTypeNameFromEnum (vr_event.eventType));
      break;
    }
  }

  if (shutdown_event_pending)
    {
      OpenVRQuitEvent *event = g_malloc (sizeof (OpenVRQuitEvent));
      event->reason = VR_QUIT_SHUTDOWN;
      g_signal_emit (self, context_signals[QUIT_EVENT], 0, event);
    }
}

void
openvr_context_show_system_keyboard (OpenVRContext *self)
{
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
  gxr_math_graphene_to_matrix34 (transform, &openvr_transform);
  self->f.overlay->SetKeyboardTransformAbsolute (self->origin,
                                                &openvr_transform);
}

void
openvr_context_acknowledge_quit (OpenVRContext *self)
{
  self->f.system->AcknowledgeQuit_Exiting ();
}

gboolean
openvr_context_is_another_scene_running (void)
{
  OpenVRContext *ctx = openvr_context_new ();
  openvr_context_initialize (ctx, OPENVR_APP_BACKGROUND);

  /* if applications fntable is not loaded, SteamVR is probably not running. */
  if (ctx->f.applications == NULL)
    return FALSE;

  uint32_t pid = ctx->f.applications->GetCurrentSceneProcessId ();

  g_object_unref (ctx);

  return pid != 0;
}

OpenVRFunctions*
openvr_context_get_functions (OpenVRContext *self)
{
  return &self->f;
}

OpenVRFunctions*
openvr_get_functions (void)
{
  OpenVRContext *self = openvr_context_get_instance ();
  return openvr_context_get_functions (self);
}

enum ETrackingUniverseOrigin
openvr_context_get_origin (OpenVRContext *self)
{
  return self->origin;
}
