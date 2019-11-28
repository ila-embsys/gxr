/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_CONTEXT_PRIVATE_H_
#define GXR_CONTEXT_PRIVATE_H_

#include "openvr-wrapper.h"
#include "openvr-context.h"

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

OpenVRFunctions*
openvr_get_functions (void);

G_END_DECLS

#endif /* GXR_CONTEXT_PRIVATE_H_ */
