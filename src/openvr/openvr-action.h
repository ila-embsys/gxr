/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENVR_ACTION_H_
#define GXR_OPENVR_ACTION_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <graphene.h>

#include "gxr-action.h"
#include "openvr-interface.h"
#include "openvr-action-set.h"

G_BEGIN_DECLS

OpenVRAction *openvr_action_new (void);

void
openvr_action_update_input_handles (OpenVRAction *self);

G_END_DECLS

#endif /* GXR_OPENVR_ACTION_H_ */
