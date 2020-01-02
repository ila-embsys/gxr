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

#include "gxr-action-set.h"
#include "gxr-enums.h"

G_BEGIN_DECLS


#define OPENXR_TYPE_ACTION_SET openxr_action_set_get_type()
G_DECLARE_FINAL_TYPE (OpenXRActionSet, openxr_action_set,
                      OPENXR, ACTION_SET, GxrActionSet)

OpenXRActionSet *openxr_action_set_new (void);

OpenXRActionSet *
openxr_action_set_new_from_url (gchar *url);

G_END_DECLS

#endif /* GXR_OPENXR_ACTION_SET_H_ */
