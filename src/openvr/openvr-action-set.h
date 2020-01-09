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

#define OPENVR_TYPE_ACTION_SET openvr_action_set_get_type()
G_DECLARE_FINAL_TYPE (OpenVRActionSet, openvr_action_set,
                      OPENVR, ACTION_SET, GxrActionSet)
OpenVRActionSet *
openvr_action_set_new_from_url (OpenVRContext *context, gchar *url);

OpenVRActionSet *openvr_action_set_new (OpenVRContext *context);

VRActionSetHandle_t
openvr_action_set_get_handle (OpenVRActionSet *self);

const gchar*
openvr_input_error_string (EVRInputError err);

G_END_DECLS

#endif /* GXR_OPENVR_ACTION_SET_H_ */
