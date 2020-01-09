/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENVR_CONTEXT_H_
#define GXR_OPENVR_CONTEXT_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <graphene.h>

#include <stdint.h>

#include "gxr-enums.h"
#include "gxr-context.h"

#include "openvr-interface.h"

#include "openvr-wrapper.h"

G_BEGIN_DECLS

typedef struct
{
  struct VR_IVRSystem_FnTable *system;
  struct VR_IVROverlay_FnTable *overlay;
  struct VR_IVRCompositor_FnTable *compositor;
  struct VR_IVRInput_FnTable *input;
  struct VR_IVRRenderModels_FnTable *model;
  struct VR_IVRApplications_FnTable *applications;
} OpenVRFunctions;

gboolean
openvr_context_is_installed (void);

gboolean
openvr_context_is_hmd_present (void);

OpenVRFunctions*
openvr_get_functions (void);

G_END_DECLS

#endif /* GXR_OPENVR_CONTEXT_H_ */
