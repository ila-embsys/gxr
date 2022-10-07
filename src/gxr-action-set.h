/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_ACTION_SET_H_
#define GXR_ACTION_SET_H_

#if !defined(GXR_INSIDE) && !defined(GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <stdint.h>

#include "gxr-action.h"
#include "gxr-manifest.h"

G_BEGIN_DECLS

#define GXR_TYPE_ACTION_SET gxr_action_set_get_type ()
G_DECLARE_FINAL_TYPE (GxrActionSet, gxr_action_set, GXR, ACTION_SET, GObject)

/**
 * GxrActionSetClass:
 * @parent: The parent class
 */
struct _GxrActionSetClass
{
  GObjectClass parent;
};

GxrActionSet *
gxr_action_set_new_from_url (GxrContext  *context,
                             GxrManifest *manifest,
                             gchar       *url);

gboolean
gxr_action_sets_poll (GxrActionSet **sets, uint32_t count);

gboolean
gxr_action_set_connect (GxrActionSet *self,
                        GxrActionType type,
                        gchar        *url,
                        GCallback     callback,
                        gpointer      data);

GxrAction *
gxr_action_set_connect_digital_from_float (GxrActionSet *self,
                                           gchar        *url,
                                           float         threshold,
                                           char         *haptic_url,
                                           GCallback     callback,
                                           gpointer      data);

GSList *
gxr_action_set_get_actions (GxrActionSet *self);

XrActionSet
gxr_action_set_get_handle (GxrActionSet *self);

GxrManifest *
gxr_action_set_get_manifest (GxrActionSet *self);

G_END_DECLS

#endif /* GXR_ACTION_SET_H_ */
