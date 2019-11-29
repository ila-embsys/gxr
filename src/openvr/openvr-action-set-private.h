/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_ACTION_SET_PRIVATE_H_
#define GXR_ACTION_SET_PRIVATE_H_

#include <glib-object.h>

#include "openvr-action-set.h"
#include "openvr-wrapper.h"

VRActionSetHandle_t
openvr_action_set_get_handle (OpenVRActionSet *self);

const gchar*
openvr_input_error_string (EVRInputError err);

#endif /* GXR_ACTION_SET_PRIVATE_H_ */
