/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_ACTION_H_
#define GXR_ACTION_H_

#if !defined(GXR_INSIDE) && !defined(GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <openxr/openxr.h>

#include "gxr-controller.h"

G_BEGIN_DECLS

#define GXR_TYPE_ACTION gxr_action_get_type ()
G_DECLARE_FINAL_TYPE (GxrAction, gxr_action, GXR, ACTION, GObject)

#ifndef __GTK_DOC_IGNORE__
typedef struct _GxrContext   GxrContext;
typedef struct _GxrActionSet GxrActionSet;
#endif

/**
 * GxrActionClass:
 * @parent: The parent class
 */
struct _GxrActionClass
{
  GObjectClass parent;
};

/**
 * GxrActionType:
 * @GXR_ACTION_DIGITAL: A digital action.
 * @GXR_ACTION_DIGITAL_FROM_FLOAT: A digital action constructed from float
 *thresholds.
 * @GXR_ACTION_VEC2F: An analog action with floats x,y.
 * @GXR_ACTION_FLOAT: An analog action.
 * @GXR_ACTION_POSE: A pose action.
 * @GXR_ACTION_HAPTIC: A haptic action.
 *
 * The type of the GxrAction.
 *
 **/
typedef enum
{
  GXR_ACTION_DIGITAL,
  GXR_ACTION_DIGITAL_FROM_FLOAT,
  GXR_ACTION_VEC2F,
  GXR_ACTION_FLOAT,
  GXR_ACTION_POSE,
  GXR_ACTION_HAPTIC
} GxrActionType;

/**
 * gxr_action_new:
 * @context: A #GxrContext.
 * @returns: A newly allocated #GxrAction.
 *
 * Creates a new action.
 */
GxrAction *
gxr_action_new (GxrContext *context);

/**
 * gxr_action_new_from_type_url:
 * @context: A #GxrContext.
 * @action_set: The action set.
 * @type: The type of the action.
 * @url: The URL of the action.
 * @returns: A newly allocated #GxrAction.
 *
 * Creates a new action from a type and URL.
 */
GxrAction *
gxr_action_new_from_type_url (GxrContext   *context,
                              GxrActionSet *action_set,
                              GxrActionType type,
                              char         *url);

/**
 * gxr_action_poll:
 * @self: A #GxrAction.
 * @returns: `TRUE` if the action was polled successfully, `FALSE` otherwise.
 *
 * Polls for events from the action.
 */
gboolean
gxr_action_poll (GxrAction *self);

/**
 * gxr_action_trigger_haptic:
 * @self: A #GxrAction.
 * @start_seconds_from_now: The start time in seconds from now.
 * @duration_seconds: The duration in seconds.
 * @frequency: The frequency in Hz.
 * @amplitude: The amplitude.
 * @controller_handle: The handle of the controller.
 * @returns: `TRUE` if the haptic feedback was triggered successfully, `FALSE` otherwise.
 *
 * Triggers haptic feedback on the action.
 */
gboolean
gxr_action_trigger_haptic (GxrAction *self,
                           float      start_seconds_from_now,
                           float      duration_seconds,
                           float      frequency,
                           float      amplitude,
                           guint64    controller_handle);

/**
 * gxr_action_get_action_type:
 * @self: A #GxrAction.
 * @returns: The type of the action.
 *
 * Gets the type of the action.
 */
GxrActionType
gxr_action_get_action_type (GxrAction *self);

/**
 * gxr_action_get_action_set:
 * @self: A #GxrAction.
 * @returns: (transfer none): The action set associated with the action.
 *
 * Gets the action set associated with the action.
 */
GxrActionSet *
gxr_action_get_action_set (GxrAction *self);

/**
 * gxr_action_get_url:
 * @self: A #GxrAction.
 * @returns: The URL of the action.
 *
 * Gets the URL of the action.
 */
gchar *
gxr_action_get_url (GxrAction *self);

/**
 * gxr_action_set_action_type:
 * @self: A #GxrAction.
 * @type: The type of the action.
 *
 * Sets the type of the action.
 */
void
gxr_action_set_action_type (GxrAction *self, GxrActionType type);

/**
 * gxr_action_set_action_set:
 * @self: A #GxrAction.
 * @action_set: The action set.
 *
 * Sets the action set associated with the action.
 */
void
gxr_action_set_action_set (GxrAction *self, GxrActionSet *action_set);

/**
 * gxr_action_set_url:
 * @self: A #GxrAction.
 * @url: The URL of the action.
 *
 * Sets the URL of the action.
 */
void
gxr_action_set_url (GxrAction *self, gchar *url);
/**
 * gxr_action_emit_digital:
 * @self: A #GxrAction.
 * @event: The digital event.
 *
 * Emits a digital event for the action.
 */

void
gxr_action_emit_digital (GxrAction *self, GxrDigitalEvent *event);

/**
 * gxr_action_emit_analog:
 * @self: A #GxrAction.
 * @event: The analog event.
 *
 * Emits an analog event for the action.
 */
void
gxr_action_emit_analog (GxrAction *self, GxrAnalogEvent *event);

/**
 * gxr_action_emit_pose:
 * @self: A #GxrAction.
 * @event: The pose event.
 *
 * Emits a pose event for the action.
 */
void
gxr_action_emit_pose (GxrAction *self, GxrPoseEvent *event);

/**
 * gxr_action_set_digital_from_float_threshold:
 * @self: A #GxrAction.
 * @threshold: The threshold for the digital event.
 *
 * Sets the threshold for the digital event.
 */
void
gxr_action_set_digital_from_float_threshold (GxrAction *self, float threshold);

/**
 * gxr_action_set_digital_from_float_haptic:
 * @self: A #GxrAction.
 * @haptic_action: The haptic action.
 *
 * Sets the digital event from the haptic action.
 */
void
gxr_action_set_digital_from_float_haptic (GxrAction *self,
                                          GxrAction *haptic_action);

/**
 * gxr_action_update_controllers:
 * @self: A #GxrAction.
 *
 * Updates the controllers associated with the action.
 */
void
gxr_action_update_controllers (GxrAction *self);

/**
 * gxr_action_get_num_bindings:
 * @self: A #GxrAction.
 * @returns: The number of bindings for the action.
 */
uint32_t
gxr_action_get_num_bindings (GxrAction *self);

/**
 * gxr_action_set_bindings:
 * @self: A #GxrAction.
 * @bindings: An array of bindings.
 *
 * Sets the bindings for the action.
 */
void
gxr_action_set_bindings (GxrAction *self, XrActionSuggestedBinding *bindings);

/**
 * gxr_action_get_handle:
 * @self: A #GxrAction.
 * @returns: The handle of the action.
 */
XrAction
gxr_action_get_handle (GxrAction *self);

/**
 * gxr_action_get_haptic_action:
 * @self: A #GxrAction.
 * @returns: (transfer none): The haptic action associated with the action.
 *
 * Gets the haptic action associated with the action.
 */
GxrAction *
gxr_action_get_haptic_action (GxrAction *self);

G_END_DECLS

#endif /* GXR_ACTION_H_ */
