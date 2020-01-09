/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENVR_INTERFACE_H_
#define GXR_OPENVR_INTERFACE_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include "gxr-context.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_CONTEXT openvr_context_get_type()
G_DECLARE_FINAL_TYPE (OpenVRContext, openvr_context,
                      OPENVR, CONTEXT, GxrContext)
OpenVRContext *openvr_context_new (void);

G_END_DECLS

#endif /* GXR_OPENVR_INTERFACE_H_ */
