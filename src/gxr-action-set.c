/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-action-set.h"
#include "gxr-action.h"

typedef struct _GxrActionSetPrivate
{
  GObject parent;

  GSList *actions;
} GxrActionSetPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GxrActionSet, gxr_action_set, G_TYPE_OBJECT)

static void
gxr_action_set_finalize (GObject *gobject);

static void
gxr_action_set_class_init (GxrActionSetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gxr_action_set_finalize;
}

static void
gxr_action_set_init (GxrActionSet *self)
{
  GxrActionSetPrivate *priv = gxr_action_set_get_instance_private (self);
  priv->actions = NULL;
}

GxrActionSet *
gxr_action_set_new (void)
{
  return (GxrActionSet*) g_object_new (GXR_TYPE_ACTION_SET, 0);
}

static void
gxr_action_set_finalize (GObject *gobject)
{
  GxrActionSet *self = GXR_ACTION_SET (gobject);
  GxrActionSetPrivate *priv = gxr_action_set_get_instance_private (self);

  g_slist_free (priv->actions);
}

gboolean
gxr_action_sets_poll (GxrActionSet **sets, uint32_t count)
{
  GxrActionSetClass *klass = GXR_ACTION_SET_GET_CLASS (sets[0]);
  if (klass->update == NULL)
      return FALSE;

  if (!klass->update (sets, count))
    return FALSE;

  for (uint32_t i = 0; i < count; i++)
    {
      GxrActionSetPrivate *priv = gxr_action_set_get_instance_private (sets[i]);
      for (GSList *l = priv->actions; l != NULL; l = l->next)
        {
          GxrAction *action = (GxrAction*) l->data;

          if (!gxr_action_poll (action))
            return FALSE;
        }
    }
  return TRUE;
}

gboolean
gxr_action_set_connect (GxrActionSet *self,
                        GxrActionType type,
                        gchar        *url,
                        GCallback     callback,
                        gpointer      data)
{
  GxrActionSetPrivate *priv = gxr_action_set_get_instance_private (self);

  GxrActionSetClass *klass = GXR_ACTION_SET_GET_CLASS (self);
  if (klass->create_action == NULL)
      return FALSE;
  GxrAction *action = klass->create_action (self, type, url);

  if (action != NULL)
    priv->actions = g_slist_append (priv->actions, action);

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


GSList *
gxr_action_set_get_actions (GxrActionSet *self)
{
  GxrActionSetPrivate *priv = gxr_action_set_get_instance_private (self);
  return priv->actions;
}