/*
 * gxr
 * Copyright 2018-2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OVERLAY_MODEL_H_
#define GXR_OVERLAY_MODEL_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include "gxr-overlay.h"

G_BEGIN_DECLS

#define GXR_TYPE_OVERLAY_MODEL gxr_overlay_model_get_type()
G_DECLARE_DERIVABLE_TYPE (GxrOverlayModel, gxr_overlay_model, GXR,
                          OVERLAY_MODEL, GObject)

/**
 * GxrOverlayModelClass:
 * @parent: The object class structure needs to be the first
 *   element in the widget class structure in order for the class mechanism
 *   to work correctly. This allows a GxrOverlayModelClass pointer to be cast to
 *   a GxrOverlayClass pointer.
 */
struct _GxrOverlayModelClass
{
  GObjectClass parent;
};

GxrOverlayModel *gxr_overlay_model_new (gchar* key, gchar* name);

gboolean
gxr_overlay_model_set_model (GxrOverlayModel *self, gchar *name,
                             graphene_vec4_t *color);

gboolean
gxr_overlay_model_get_model (GxrOverlayModel *self, gchar *name,
                             graphene_vec4_t *color, uint32_t *id);

gboolean
gxr_overlay_model_initialize (GxrOverlayModel *self, gchar* key, gchar* name);

GxrOverlay*
gxr_overlay_model_get_overlay (GxrOverlayModel *self);

G_END_DECLS

#endif /* GXR_OVERLAY_MODEL_H_ */
