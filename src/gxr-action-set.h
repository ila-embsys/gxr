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

/**
 * gxr_action_set_new_from_url:
 * @context: A #GxrContext.
 * @manifest: The manifest.
 * @url: The URL of the action set.
 * @returns: A newly allocated #GxrActionSet.
 *
 * Creates a new action set from a URL.
 */
GxrActionSet *
gxr_action_set_new_from_url (GxrContext  *context,
                             GxrManifest *manifest,
                             gchar       *url);

/**
 * gxr_action_sets_poll:
 * @sets: An array of action sets.
 * @count: The number of action sets.
 * @returns: `TRUE` if the action sets were polled successfully, `FALSE` otherwise.
 *
 * Polls for events from the action sets.
 */
gboolean
gxr_action_sets_poll (GxrActionSet **sets, uint32_t count);

/**
 * gxr_action_set_connect:
 * @self: A #GxrActionSet.
 * @type: The type of the action.
 * @url: The URL of the action.
 * @callback: (scope call): The callback function.
 * @data: The user data for the callback.
 * @returns: `TRUE` if the action was connected successfully, `FALSE` otherwise.
 *
 * Connects an action to the action set.
 */
gboolean
gxr_action_set_connect (GxrActionSet *self,
                        GxrActionType type,
                        gchar        *url,
                        GCallback     callback,
                        gpointer      data);

/**
 * gxr_action_set_connect_digital_from_float:
 * @self: A #GxrActionSet.
 * @url: The URL of the action.
 * @threshold: The threshold for the digital event.
 * @haptic_url: The URL of the haptic action.
 * @callback: (scope call): The callback function.
 * @data: The user data for the callback.
 * @returns: (transfer none): The created action.
 *
 * Creates a new digital action from a float threshold and connects it to the action set.
 */
GxrAction *
gxr_action_set_connect_digital_from_float (GxrActionSet *self,
                                           gchar        *url,
                                           float         threshold,
                                           char         *haptic_url,
                                           GCallback     callback,
                                           gpointer      data);

/**
 * gxr_action_set_get_actions:
 * @self: A #GxrActionSet.
 * @returns: (element-type GxrAction) (transfer container): A newly allocated #GSList containing the actions in the action set.
 *
 * Gets the actions in the action set.
 */
GSList *
gxr_action_set_get_actions (GxrActionSet *self);

/**
 * gxr_action_set_get_handle:
 * @self: A #GxrActionSet.
 * @returns: The handle of the action set.
 */
XrActionSet
gxr_action_set_get_handle (GxrActionSet *self);

/**
 * gxr_action_set_get_manifest:
 * @self: A #GxrActionSet.
 * @returns: (transfer none): The manifest associated with the action set.
 *
 * Gets the manifest associated with the action set.
 */
GxrManifest *
gxr_action_set_get_manifest (GxrActionSet *self);

/**
 * gxr_action_set_append_action:
 * @self: A #GxrActionSet.
 * @action: The action to append.
 *
 * Appends an action to the action set.
 */
void
gxr_action_set_append_action (GxrActionSet *self, GxrAction *action);

G_END_DECLS

#endif /* GXR_ACTION_SET_H_ */
