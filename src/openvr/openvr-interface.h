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

#include "openvr-context.h"
#include "openvr-overlay.h"
#include "openvr-action.h"

G_BEGIN_DECLS

OpenVRContext *openvr_context_new (void);
OpenVROverlay *openvr_overlay_new (void);

OpenVRAction *
openvr_action_new_from_type_url (GxrActionSet *action_set,
                                 GxrActionType type, char *url);

G_END_DECLS

#endif /* GXR_OPENVR_INTERFACE_H_ */
