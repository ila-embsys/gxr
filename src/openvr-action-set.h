/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_ACTION_SET_H_
#define GXR_ACTION_SET_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <stdint.h>

#include "gxr-enums.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_ACTION_SET openvr_action_set_get_type()
G_DECLARE_FINAL_TYPE (OpenVRActionSet, openvr_action_set,
                      OPENVR, ACTION_SET, GObject)

OpenVRActionSet *openvr_action_set_new (void);

OpenVRActionSet *
openvr_action_set_new_from_url (gchar *url);

gboolean
openvr_action_sets_poll (OpenVRActionSet **sets, uint32_t count);

gboolean
openvr_action_set_connect (OpenVRActionSet *self,
                           GxrActionType    type,
                           gchar           *url,
                           GCallback        callback,
                           gpointer         data);

G_END_DECLS

#endif /* GXR_ACTION_SET_H_ */
