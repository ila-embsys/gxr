/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_CONTEXT_H_
#define GXR_CONTEXT_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <graphene.h>

#include "gxr-enums.h"

G_BEGIN_DECLS

#define GXR_TYPE_CONTEXT gxr_context_get_type()
G_DECLARE_FINAL_TYPE (GxrContext, gxr_context, GXR, CONTEXT, GObject)

// GxrContext *gxr_context_new (void);

GxrContext *gxr_context_get_instance (void);

GxrApi
gxr_context_get_api (GxrContext *self);

gboolean
gxr_context_get_head_pose (graphene_matrix_t *pose);

void
gxr_context_get_frustum_angles (GxrEye eye,
                                float *left, float *right,
                                float *top, float *bottom);

gboolean
gxr_context_is_input_available (void);

G_END_DECLS

#endif /* GXR_CONTEXT_H_ */
