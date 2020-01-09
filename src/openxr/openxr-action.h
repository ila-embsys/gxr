/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Christoph Haag <christop.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENXR_ACTION_H_
#define GXR_OPENXR_ACTION_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <graphene.h>

#include "gxr-action.h"
#include "openxr-action-set.h"
#include <openxr/openxr.h>

G_BEGIN_DECLS

#define OPENXR_TYPE_ACTION openxr_action_get_type()
G_DECLARE_FINAL_TYPE (OpenXRAction, openxr_action, OPENXR, ACTION, GxrAction)

gboolean
openxr_action_load_manifest (char *path);

OpenXRAction *openxr_action_new (void);

void
openxr_action_update_input_handles (OpenXRAction *self);

gboolean
openxr_action_load_cached_manifest (const char* cache_name,
                                    const char* resource_path,
                                    const char* manifest_name,
                                    const char* first_binding,
                                    ...);

uint32_t
openxr_action_get_num_bindings (OpenXRAction *self);

void
openxr_action_set_bindings (OpenXRAction *self,
                            XrActionSuggestedBinding *bindings);

char *
openxr_action_get_url (OpenXRAction *self);

XrAction
openxr_action_get_handle (OpenXRAction *self);

G_END_DECLS

#endif /* GXR_OPENXR_ACTION_H_ */
