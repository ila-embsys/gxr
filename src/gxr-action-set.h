/*
 * gxr
 * Copyright 2019 Collabora Ltd.
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

#include "gxr-action.h"

G_BEGIN_DECLS

#define GXR_TYPE_ACTION_SET gxr_action_set_get_type()
G_DECLARE_DERIVABLE_TYPE (GxrActionSet, gxr_action_set,
                          GXR, ACTION_SET, GObject)

struct _GxrActionSetClass
{
  GObjectClass parent;

  gboolean
  (*update) (GxrActionSet **sets, uint32_t count);

  GxrAction*
  (*create_action) (GxrActionSet *self,
                    GxrActionType type, char *url);

  gboolean
  (*attach_bindings) (GxrActionSet *self);

};

GxrActionSet *gxr_action_set_new (void);

gboolean
gxr_action_sets_poll (GxrActionSet **sets, uint32_t count);

gboolean
gxr_action_set_connect (GxrActionSet *self,
                        GxrActionType type,
                        gchar        *url,
                        GCallback     callback,
                        gpointer      data);

GSList *
gxr_action_set_get_actions (GxrActionSet *self);

gboolean
gxr_action_set_attach_bindings (GxrActionSet *self);

G_END_DECLS

#endif /* GXR_ACTION_SET_H_ */
