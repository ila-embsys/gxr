/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_ACTION_SET_PRIVATE_H_
#define GXR_ACTION_SET_PRIVATE_H_

#include "openxr-action-set.h"
#include "openxr/openxr.h"

XrActionSet
openxr_action_set_get_handle (OpenXRActionSet *self);

#endif /* GXR_ACTION_SET_PRIVATE_H_ */
