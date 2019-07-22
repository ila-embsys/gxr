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

#include "openvr-wrapper.h"

G_BEGIN_DECLS

/**
 * OpenVRActionType:
 * @OPENVR_ACTION_DIGITAL: A digital action.
 * @OPENVR_ACTION_ANALOG: An analog action.
 * @OPENVR_ACTION_POSE: A pose action.
 *
 * The type of the OpenVR action.
 *
 **/
typedef enum OpenVRActionType {
  OPENVR_ACTION_DIGITAL,
  OPENVR_ACTION_ANALOG,
  OPENVR_ACTION_POSE
} OpenVRActionType;

#define OPENVR_TYPE_ACTION_SET openvr_action_set_get_type()
G_DECLARE_FINAL_TYPE (OpenVRActionSet, openvr_action_set,
                      OPENVR, ACTION_SET, GObject)

OpenVRActionSet *openvr_action_set_new (void);

OpenVRActionSet *
openvr_action_set_new_from_url (gchar *url);

gboolean
openvr_action_set_update (OpenVRActionSet *self);

gboolean
openvr_action_set_poll (OpenVRActionSet *self);

gboolean
openvr_action_set_connect (OpenVRActionSet *self,
                           OpenVRActionType type,
                           gchar           *url,
                           GCallback        callback,
                           gpointer         data);

VRActionSetHandle_t
openvr_action_set_get_handle (OpenVRActionSet *self);

G_END_DECLS

#endif /* GXR_ACTION_SET_H_ */
