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
  GxrActionSet parent;

  VRActionSetHandle_t handle;
};

G_DEFINE_TYPE (OpenVRActionSet, openvr_action_set, GXR_TYPE_ACTION_SET)

VRActionSetHandle_t
openvr_action_set_get_handle (OpenVRActionSet *self)
{
  return self->handle;
}

static void
openvr_action_set_finalize (GObject *gobject);

static gboolean
_update (GxrActionSet **sets, uint32_t count);

static void
_update_input_handles (OpenVRActionSet *self)
{
  GxrActionSet *self_gxr = GXR_ACTION_SET (self);
  if (!_update (&self_gxr, 1))
    {
      g_print ("Failed to update Action Set after binding update!\n");
      return;
    }

  GSList *actions = gxr_action_set_get_actions (GXR_ACTION_SET (self));
  for (GSList *l = actions; l != NULL; l = l->next)
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

  OpenVRContext *context = openvr_context_get_instance ();
  g_signal_connect (context, "binding-loaded-event",
                    (GCallback) _binding_loaded_cb, self);
}

OpenVRActionSet *
openvr_action_set_new (void)
{
  return (OpenVRActionSet*) g_object_new (OPENVR_TYPE_ACTION_SET, 0);
}

gboolean
_load_handle (OpenVRActionSet *self,
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

OpenVRActionSet *
openvr_action_set_new_from_url (gchar *url)
{
  OpenVRActionSet *self = openvr_action_set_new ();
  if (!_load_handle (self, url))
    {
      g_object_unref (self);
      self = NULL;
    }
  return self;
}

static void
openvr_action_set_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (openvr_action_set_parent_class)->finalize (gobject);
}

static gboolean
_update (GxrActionSet **sets, uint32_t count)
{
  struct VRActiveActionSet_t *active_action_sets =
    g_malloc (sizeof (struct VRActiveActionSet_t) * count);

  for (uint32_t i = 0; i < count; i++)
    {
      active_action_sets[i].ulActionSet = OPENVR_ACTION_SET (sets[i])->handle;
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

GxrAction*
_create_action (GxrActionSet *self,
                GxrActionType type, char *url)
{
  return (GxrAction*) openvr_action_new_from_type_url (self, type, url);
}

static void
openvr_action_set_class_init (OpenVRActionSetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_action_set_finalize;

  GxrActionSetClass *gxr_action_set_class = GXR_ACTION_SET_CLASS (klass);
  gxr_action_set_class->update = _update;
  gxr_action_set_class->create_action = _create_action;
}
