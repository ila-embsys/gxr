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

#include "gxr-context.h"
#include "gxr-overlay.h"
#include "gxr-action.h"

G_BEGIN_DECLS

#define OPENXR_TYPE_CONTEXT openxr_context_get_type()
G_DECLARE_FINAL_TYPE (OpenXRContext, openxr_context,
                      OPENXR, CONTEXT, GxrContext)
OpenXRContext *openxr_context_new (void);

#define OPENXR_TYPE_OVERLAY openxr_overlay_get_type()
G_DECLARE_FINAL_TYPE (OpenXROverlay, openxr_overlay, OPENXR, OVERLAY, GxrOverlay)
OpenXROverlay *openxr_overlay_new (void);

#define OPENXR_TYPE_ACTION openxr_action_get_type()
G_DECLARE_FINAL_TYPE (OpenXRAction, openxr_action, OPENXR, ACTION, GxrAction)
OpenXRAction *
openxr_action_new_from_type_url (GxrActionSet *action_set,
                                 GxrActionType type, char *url);

#define OPENXR_TYPE_ACTION_SET openxr_action_set_get_type()
G_DECLARE_FINAL_TYPE (OpenXRActionSet, openxr_action_set,
                      OPENXR, ACTION_SET, GxrActionSet)
OpenXRActionSet *
openxr_action_set_new_from_url (gchar *url);

G_END_DECLS

#endif /* GXR_OPENXR_INTERFACE_H_ */
