/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-context.h"

#include <glib/gprintf.h>
#include "openvr_capi_global.h"

G_DEFINE_TYPE (OpenVRContext, openvr_context, G_TYPE_OBJECT)

#define INIT_FN_TABLE(target, type) \
{ \
  intptr_t ptr = 0; \
  gboolean ret = _init_fn_table (IVR##type##_Version, &ptr); \
  if (!ret || ptr == 0) \
    return false; \
  target = (struct VR_IVR##type##_FnTable*) ptr; \
}

static void
openvr_context_finalize (GObject *gobject);

static void
openvr_context_class_init (OpenVRContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = openvr_context_finalize;
}

static void
openvr_context_init (OpenVRContext *self)
{
  self->system = NULL;
  self->overlay = NULL;
  self->compositor = NULL;
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

OpenVRContext *
openvr_context_get_instance ()
{
  static OpenVRContext *self = NULL;

  if (self == NULL)
    self = openvr_context_new ();

  return self;
}

gboolean
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

bool
_init_function_tables (OpenVRContext *self)
{
  INIT_FN_TABLE (self->system, System);
  INIT_FN_TABLE (self->overlay, Overlay);
  INIT_FN_TABLE (self->compositor, Compositor);
  INIT_FN_TABLE (self->input, Input);
  return true;
}

/* TODO: Create app type enum to make this public */
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

gboolean
openvr_context_init_overlay (OpenVRContext * self)
{
  return _vr_init (self, EVRApplicationType_VRApplication_Overlay);
}

gboolean
openvr_context_is_valid (OpenVRContext * self)
{
  return self->system != NULL
    && self->overlay != NULL
    && self->compositor != NULL
    && self->input != NULL;
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
