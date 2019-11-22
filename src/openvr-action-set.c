/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-action-set.h"
#include "openvr-action-set-private.h"

#include "openvr-context-private.h"
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

VRActionSetHandle_t
openvr_action_set_get_handle (OpenVRActionSet *self)
{
  return self->handle;
}

static void
openvr_action_set_finalize (GObject *gobject);

static void
openvr_action_set_class_init (OpenVRActionSetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_action_set_finalize;
}

static gboolean
_action_sets_update (OpenVRActionSet **sets, uint32_t count);

static void
_update_input_handles (OpenVRActionSet *self)
{
  if (!_action_sets_update (&self, 1))
    {
      g_print ("Failed to update Action Set after binding update!\n");
      return;
    }

  for (GSList *l = self->actions; l != NULL; l = l->next)
    {
      OpenVRAction *action = (OpenVRAction*) l->data;
      openvr_action_update_input_handles (action);
    }
}

static void
_binding_loaded_cb (OpenVRContext   *context,
                    OpenVRActionSet *self)
{
  (void) context;
  _update_input_handles (self);
}

static void
openvr_action_set_init (OpenVRActionSet *self)
{
  self->handle = k_ulInvalidActionSetHandle;
  self->actions = NULL;

  OpenVRContext *context = openvr_context_get_instance ();
  g_signal_connect (context, "binding-loaded-event",
                    (GCallback) _binding_loaded_cb, self);
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
  OpenVRFunctions *f = openvr_context_get_functions (context);

  err = f->input->GetActionSetHandle (url, &self->handle);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: GetActionSetHandle: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }

  return TRUE;
}

static gboolean
_action_sets_update (OpenVRActionSet **sets, uint32_t count)
{
  struct VRActiveActionSet_t *active_action_sets =
    g_malloc (sizeof (struct VRActiveActionSet_t) * count);

  for (uint32_t i = 0; i < count; i++)
    {
      active_action_sets[i].ulActionSet = sets[i]->handle;
      active_action_sets[i].ulRestrictedToDevice = 0;
      active_action_sets[i].ulSecondaryActionSet = 0;
      active_action_sets[i].unPadding = 0;
      active_action_sets[i].nPriority = 0;
    }

  OpenVRContext *context = openvr_context_get_instance ();
  OpenVRFunctions *f = openvr_context_get_functions (context);

  EVRInputError err;
  err = f->input->UpdateActionState (active_action_sets,
                                     sizeof (struct VRActiveActionSet_t),
                                     count);

  g_free (active_action_sets);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: UpdateActionState: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }

  return TRUE;
}

gboolean
openvr_action_sets_poll (OpenVRActionSet **sets, uint32_t count)
{
  if (!_action_sets_update (sets, count))
    return FALSE;

  for (uint32_t i = 0; i < count; i++)
    {
      for (GSList *l = sets[i]->actions; l != NULL; l = l->next)
        {
          OpenVRAction *action = (OpenVRAction*) l->data;

          if (!openvr_action_poll (action))
            return FALSE;
        }
    }

  return TRUE;
}

gboolean
openvr_action_set_connect (OpenVRActionSet *self,
                           GxrActionType    type,
                           gchar           *url,
                           GCallback        callback,
                           gpointer         data)
{
  OpenVRAction *action = openvr_action_new_from_type_url (self, type, url);

  if (action != NULL)
    self->actions = g_slist_append (self->actions, action);

  switch (type)
    {
    case GXR_ACTION_DIGITAL:
      g_signal_connect (action, "digital-event", callback, data);
      break;
    case GXR_ACTION_ANALOG:
      g_signal_connect (action, "analog-event", callback, data);
      break;
    case GXR_ACTION_POSE:
      g_signal_connect (action, "pose-event", callback, data);
      break;
    default:
      g_printerr ("Uknown action type %d\n", type);
      return FALSE;
    }

  return TRUE;

}

#define ENUM_TO_STR(r) case r: return #r

const gchar*
openvr_input_error_string (EVRInputError err)
{
  switch (err)
    {
      ENUM_TO_STR(EVRInputError_VRInputError_None);
      ENUM_TO_STR(EVRInputError_VRInputError_NameNotFound);
      ENUM_TO_STR(EVRInputError_VRInputError_WrongType);
      ENUM_TO_STR(EVRInputError_VRInputError_InvalidHandle);
      ENUM_TO_STR(EVRInputError_VRInputError_InvalidParam);
      ENUM_TO_STR(EVRInputError_VRInputError_NoSteam);
      ENUM_TO_STR(EVRInputError_VRInputError_MaxCapacityReached);
      ENUM_TO_STR(EVRInputError_VRInputError_IPCError);
      ENUM_TO_STR(EVRInputError_VRInputError_NoActiveActionSet);
      ENUM_TO_STR(EVRInputError_VRInputError_InvalidDevice);
      ENUM_TO_STR(EVRInputError_VRInputError_InvalidSkeleton);
      ENUM_TO_STR(EVRInputError_VRInputError_InvalidBoneCount);
      ENUM_TO_STR(EVRInputError_VRInputError_InvalidCompressedData);
      ENUM_TO_STR(EVRInputError_VRInputError_NoData);
      ENUM_TO_STR(EVRInputError_VRInputError_BufferTooSmall);
      ENUM_TO_STR(EVRInputError_VRInputError_MismatchedActionManifest);
      ENUM_TO_STR(EVRInputError_VRInputError_MissingSkeletonData);
      default:
        return "UNKNOWN EVRInputError";
    }
}
