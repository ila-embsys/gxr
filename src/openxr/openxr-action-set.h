/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Christoph Haag <christop.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENXR_ACTION_SET_H_
#define GXR_OPENXR_ACTION_SET_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <stdint.h>
#include <openxr/openxr.h>

#include "gxr-action-set.h"
#include "gxr-enums.h"

#include "openxr-interface.h"

G_BEGIN_DECLS

OpenXRActionSet *openxr_action_set_new (void);

XrActionSet
openxr_action_set_get_handle (OpenXRActionSet *self);

G_END_DECLS

#endif /* GXR_OPENXR_ACTION_SET_H_ */
