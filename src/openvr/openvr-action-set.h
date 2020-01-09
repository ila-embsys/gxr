/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENVR_ACTION_SET_H_
#define GXR_OPENVR_ACTION_SET_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <stdint.h>
#include <glib-object.h>

#include "gxr-action-set.h"
#include "gxr-enums.h"

#include "openvr-action-set.h"
#include "openvr-wrapper.h"
#include "openvr-interface.h"

G_BEGIN_DECLS

OpenVRActionSet *openvr_action_set_new (void);

VRActionSetHandle_t
openvr_action_set_get_handle (OpenVRActionSet *self);

const gchar*
openvr_input_error_string (EVRInputError err);

G_END_DECLS

#endif /* GXR_OPENVR_ACTION_SET_H_ */
