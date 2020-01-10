/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_BACKEND_H_
#define GXR_BACKEND_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>

#include "gxr-enums.h"

G_BEGIN_DECLS

typedef struct _GxrContext GxrContext;

#define GXR_TYPE_BACKEND gxr_backend_get_type()
G_DECLARE_FINAL_TYPE (GxrBackend, gxr_backend, GXR, BACKEND, GObject)

GxrBackend *
gxr_backend_get_instance (GxrApi api);

void
gxr_backend_shutdown (void);

GxrContext *
gxr_backend_new_context (GxrBackend *self);

G_END_DECLS

#endif /* GXR_BACKEND_H_ */
