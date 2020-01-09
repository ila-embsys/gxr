/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENXR_INTERFACE_H_
#define GXR_OPENXR_INTERFACE_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include "openxr-context.h"
#include "openxr-overlay.h"
#include "openxr-action.h"

G_BEGIN_DECLS

OpenXRContext *openxr_context_new (void);

OpenXROverlay *openxr_overlay_new (void);

OpenXRAction *
openxr_action_new_from_type_url (GxrActionSet *action_set,
                                 GxrActionType type, char *url);

G_END_DECLS

#endif /* GXR_OPENXR_INTERFACE_H_ */
