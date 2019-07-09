/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-action-set.h"

#include "openvr-context.h"
#include "openvr-action.h"

struct _OpenVRActionSet
{
  GObject parent;

  GSList *actions;

  VRActionSetHandle_t handle;
};

G_DEFINE_TYPE (OpenVRActionSet, openvr_action_set, G_TYPE_OBJECT)

gboolean
openvr_action_set_load_handle (OpenVRActionSet *self,
                               gchar           *url);

static void
openvr_action_set_finalize (GObject *gobject);

static void
openvr_action_set_class_init (OpenVRActionSetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_action_set_finalize;
}

static void
openvr_action_set_init (OpenVRActionSet *self)
{
  self->handle = k_ulInvalidActionSetHandle;
  self->actions = NULL;
}

OpenVRActionSet *
openvr_action_set_new (void)
{
  return (OpenVRActionSet*) g_object_new (OPENVR_TYPE_ACTION_SET, 0);
}

OpenVRActionSet *
openvr_action_set_new_from_url (gchar *url)
{
  OpenVRActionSet *self = openvr_action_set_new ();
  if (!openvr_action_set_load_handle (self, url))
    {
      g_object_unref (self);
      self = NULL;
    }
  return self;
}

static void
openvr_action_set_finalize (GObject *gobject)
{
  OpenVRActionSet *self = OPENVR_ACTION_SET (gobject);
  g_slist_free (self->actions);
}

gboolean
openvr_action_set_load_handle (OpenVRActionSet *self,
                               gchar           *url)
{
  EVRInputError err;

  OpenVRContext *context = openvr_context_get_instance ();

  err = context->input->GetActionSetHandle (url, &self->handle);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: GetActionSetHandle: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }

  return TRUE;
}

gboolean
openvr_action_set_poll (OpenVRActionSet *self)
{
  if (!openvr_action_set_update (self))
    return FALSE;

  for (GSList *l = self->actions; l != NULL; l = l->next)
    {
      OpenVRAction *action = (OpenVRAction*) l->data;

      /* Can't query for origins directly after creating action.
       * TODO: maybe we can do this once, but later, and only update after
       * activate-device event. */
      openvr_action_update_input_handles (action);

      if (!openvr_action_poll (action))
        return FALSE;
    }

  return TRUE;
}


gboolean
openvr_action_set_update (OpenVRActionSet *self)
{
  struct VRActiveActionSet_t active_action_set = {};
  active_action_set.ulActionSet = self->handle;

  OpenVRContext *context = openvr_context_get_instance ();

  EVRInputError err;
  err = context->input->UpdateActionState (&active_action_set,
                                           sizeof(active_action_set),
                                           1);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: UpdateActionState: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }

  return TRUE;
}

gboolean
openvr_action_set_connect (OpenVRActionSet *self,
                           OpenVRActionType type,
                           gchar           *url,
                           GCallback        callback,
                           gpointer         data)
{
  OpenVRAction *action = openvr_action_new_from_type_url (self, type, url);

  if (action != NULL)
    self->actions = g_slist_append (self->actions, action);

  switch (type)
    {
    case OPENVR_ACTION_DIGITAL:
      g_signal_connect (action, "digital-event", callback, data);
      break;
    case OPENVR_ACTION_ANALOG:
      g_signal_connect (action, "analog-event", callback, data);
      break;
    case OPENVR_ACTION_POSE:
      g_signal_connect (action, "pose-event", callback, data);
      break;
    default:
      g_printerr ("Uknown action type %d\n", type);
      return FALSE;
    }

  return TRUE;

}

VRActionSetHandle_t
openvr_action_set_get_handle (OpenVRActionSet *self)
{
  return self->handle;
}
