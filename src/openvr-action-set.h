/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_ACTION_SET_H_
#define OPENVR_GLIB_ACTION_SET_H_

#include <glib-object.h>

#include <openvr_capi.h>

#include "openvr-action.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_ACTION_SET openvr_action_set_get_type()
G_DECLARE_FINAL_TYPE (OpenVRActionSet, openvr_action_set,
                      OPENVR, ACTION_SET, GObject)

struct _OpenVRActionSet
{
  GObject parent;

  GSList *actions;

  VRActionSetHandle_t handle;
};

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

G_END_DECLS

#endif /* OPENVR_GLIB_ACTION_SET_H_ */
