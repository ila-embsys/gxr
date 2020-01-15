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
G_DECLARE_FINAL_TYPE (GxrOverlayModel, gxr_overlay_model, GXR,
                      OVERLAY_MODEL, GObject)

GxrOverlayModel *gxr_overlay_model_new (GxrContext *context, gchar* key);

gboolean
gxr_overlay_model_set_model (GxrOverlayModel *self, gchar *name,
                             graphene_vec4_t *color);

gboolean
gxr_overlay_model_get_model (GxrOverlayModel *self, gchar *name,
                             graphene_vec4_t *color, uint32_t *id);

GxrOverlay*
gxr_overlay_model_get_overlay (GxrOverlayModel *self);

G_END_DECLS

#endif /* GXR_OVERLAY_MODEL_H_ */
