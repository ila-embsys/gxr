/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_CONTEXT_PRIVATE_H_
#define GXR_CONTEXT_PRIVATE_H_

#include "gxr-context.h"

G_BEGIN_DECLS

/* Signal emission */
void
gxr_context_emit_keyboard_press (GxrContext *self, gpointer event);

void
gxr_context_emit_keyboard_close (GxrContext *self);

void
gxr_context_emit_state_change (GxrContext *self, gpointer event);

void
gxr_context_emit_overlay_event (GxrContext *self, gpointer event);

void
gxr_context_emit_device_activate (GxrContext *self, gpointer event);

void
gxr_context_emit_device_deactivate (GxrContext *self, gpointer event);

void
gxr_context_emit_device_update (GxrContext *self, gpointer event);

void
gxr_context_emit_bindings_update (GxrContext *self);

void
gxr_context_emit_binding_loaded (GxrContext *self);

void
gxr_context_emit_actionset_update (GxrContext *self);

G_END_DECLS

#endif /* GXR_CONTEXT_PRIVATE_H_ */
