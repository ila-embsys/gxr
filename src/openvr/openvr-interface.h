/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENVR_INTERFACE_H_
#define GXR_OPENVR_INTERFACE_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include "gxr-context.h"
#include "gxr-overlay.h"
#include "gxr-action.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_CONTEXT openvr_context_get_type()
G_DECLARE_FINAL_TYPE (OpenVRContext, openvr_context,
                      OPENVR, CONTEXT, GxrContext)
OpenVRContext *openvr_context_new (void);

#define OPENVR_TYPE_OVERLAY openvr_overlay_get_type()
G_DECLARE_FINAL_TYPE (OpenVROverlay, openvr_overlay, OPENVR, OVERLAY, GxrOverlay)
OpenVROverlay *openvr_overlay_new (void);

#define OPENVR_TYPE_ACTION openvr_action_get_type()
G_DECLARE_FINAL_TYPE (OpenVRAction, openvr_action, OPENVR, ACTION, GxrAction)
OpenVRAction *
openvr_action_new_from_type_url (GxrActionSet *action_set,
                                 GxrActionType type, char *url);

gboolean
openvr_action_load_manifest (const char* cache_name,
                             const char* resource_path,
                             const char* manifest_name,
                             const char* first_binding,
                             va_list     args);

#define OPENVR_TYPE_ACTION_SET openvr_action_set_get_type()
G_DECLARE_FINAL_TYPE (OpenVRActionSet, openvr_action_set,
                      OPENVR, ACTION_SET, GxrActionSet)
OpenVRActionSet *
openvr_action_set_new_from_url (gchar *url);

G_END_DECLS

#endif /* GXR_OPENVR_INTERFACE_H_ */
