/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-device.h"

#include <gxr.h>
#include <gulkan.h>

typedef struct _GxrDevicePrivate
{
  GObject parent;

  guint64 device_id;

  gboolean pose_valid;

  graphene_matrix_t transformation;

} GxrDevicePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GxrDevice, gxr_device, G_TYPE_OBJECT)

static void
gxr_device_finalize (GObject *gobject);

static void
gxr_device_class_init (GxrDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gxr_device_finalize;
}

static void
gxr_device_init (GxrDevice *self)
{
  GxrDevicePrivate *priv = gxr_device_get_instance_private (self);
  priv->pose_valid = FALSE;
}

GxrDevice *
gxr_device_new (guint64 device_id)
{
  GxrDevice *self = (GxrDevice*) g_object_new (GXR_TYPE_DEVICE, 0);
  GxrDevicePrivate *priv = gxr_device_get_instance_private (self);
  priv->device_id = device_id;
  return self;
}

static void
gxr_device_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (gxr_device_parent_class)->finalize (gobject);
}

gboolean
gxr_device_is_controller (GxrDevice *self)
{
  return GXR_IS_CONTROLLER (self);
}

void
gxr_device_set_is_pose_valid (GxrDevice *self, bool valid)
{
  GxrDevicePrivate *priv = gxr_device_get_instance_private (self);
  priv->pose_valid = valid;
}

gboolean
gxr_device_is_pose_valid (GxrDevice *self)
{
  GxrDevicePrivate *priv = gxr_device_get_instance_private (self);
  return priv->pose_valid;
}

/*
 * Set transformation without matrix decomposition and ability to rebuild
 * This will include scale as well.
 */

void
gxr_device_set_transformation_direct (GxrDevice         *self,
                                      graphene_matrix_t *mat)
{
  GxrDevicePrivate *priv = gxr_device_get_instance_private (self);
  graphene_matrix_init_from_matrix (&priv->transformation, mat);
}

void
gxr_device_get_transformation_direct (GxrDevice         *self,
                                      graphene_matrix_t *mat)
{
  GxrDevicePrivate *priv = gxr_device_get_instance_private (self);
  graphene_matrix_init_from_matrix (mat, &priv->transformation);
}

guint64
gxr_device_get_handle (GxrDevice *self)
{
  GxrDevicePrivate *priv = gxr_device_get_instance_private (self);
  return priv->device_id;
}

void
gxr_device_set_handle (GxrDevice *self, guint64 handle)
{
  GxrDevicePrivate *priv = gxr_device_get_instance_private (self);
  priv->device_id = handle;
}
