/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-pointer.h"

G_DEFINE_TYPE (OpenVRPointer, openvr_pointer, OPENVR_TYPE_MODEL)

static void
openvr_pointer_finalize (GObject *gobject);

static void
openvr_pointer_class_init (OpenVRPointerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_pointer_finalize;
}

static void
openvr_pointer_init (OpenVRPointer *self)
{
  (void) self;
}

OpenVRPointer *
openvr_pointer_new (void)
{
  OpenVRPointer *self = (OpenVRPointer*) g_object_new (OPENVR_TYPE_POINTER, 0);

  if (!openvr_model_initialize (OPENVR_MODEL (self), "pointer", "Pointer"))
    return NULL;

  /*
   * The pointer itself should always be visible on top of overlays,
   * so we use UINT32_MAX here.
   */
  openvr_overlay_set_sort_order (OPENVR_OVERLAY (self), UINT32_MAX);

  struct HmdColor_t color = {
    .r = 1.0f,
    .g = 1.0f,
    .b = 1.0f,
    .a = 1.0f
  };

  if (!openvr_model_set_model (OPENVR_MODEL (self), "{system}laser_pointer",
                              &color))
    return NULL;

  if (!openvr_overlay_set_width_meters (OPENVR_OVERLAY (self), 0.01f))
    return NULL;

  if (!openvr_overlay_show (OPENVR_OVERLAY (self)))
    return NULL;

  return self;
}

static void
openvr_pointer_finalize (GObject *gobject)
{
  OpenVRPointer *self = OPENVR_POINTER (gobject);
  (void) self;
}

void
openvr_pointer_move (OpenVRPointer     *self,
                     graphene_matrix_t *transform,
                     float              distance)
{
  graphene_matrix_t scale_matrix;
  graphene_matrix_init_scale (&scale_matrix, 1.0f, 1.0f, distance);
  graphene_matrix_t scaled;
  graphene_matrix_multiply (&scale_matrix, transform, &scaled);
  openvr_overlay_set_transform_absolute (OPENVR_OVERLAY (self), &scaled);
}
