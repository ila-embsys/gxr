/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_CONTEXT_H_
#define OPENVR_GLIB_CONTEXT_H_

#include <glib-object.h>
#include <openvr_capi.h>

G_BEGIN_DECLS

#define OPENVR_TYPE_CONTEXT openvr_context_get_type()
G_DECLARE_FINAL_TYPE (OpenVRContext, openvr_context, OPENVR, CONTEXT, GObject)

struct _OpenVRContext
{
  GObject parent;

  struct VR_IVRSystem_FnTable *system;
  struct VR_IVROverlay_FnTable *overlay;
  struct VR_IVRCompositor_FnTable *compositor;
  struct VR_IVRInput_FnTable *input;
  struct VR_IVRRenderModels_FnTable *model;
  enum ETrackingUniverseOrigin origin;
};

OpenVRContext *openvr_context_get_instance (void);

gboolean
openvr_context_init_overlay (OpenVRContext *self);

gboolean
openvr_context_is_valid (OpenVRContext *self);

gboolean
openvr_context_is_installed (void);

gboolean
openvr_context_is_hmd_present (void);

void
openvr_context_print_model_list (OpenVRContext *self);

GSList *
openvr_context_get_model_list (OpenVRContext *self);


const gchar*
openvr_input_error_string (EVRInputError err);

G_END_DECLS

#endif /* OPENVR_GLIB_CONTEXT_H_ */
