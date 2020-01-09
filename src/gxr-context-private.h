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

/* Methods exposed through other Gxr classes */
GxrOverlay *
gxr_context_new_overlay (GxrContext *self);

GxrAction *
gxr_context_new_action_from_type_url (GxrContext   *self,
                                      GxrActionSet *action_set,
                                      GxrActionType type,
                                      char          *url);

GxrActionSet *
gxr_context_new_action_set_from_url (GxrContext *self, gchar *url);

gboolean
gxr_context_load_action_manifest (GxrContext *self,
                                  const char *cache_name,
                                  const char *resource_path,
                                  const char *manifest_name,
                                  const char *first_binding,
                                  va_list     args);

/* Signal emission */
void
gxr_context_emit_keyboard_press (GxrContext *self, gpointer event);

void
gxr_context_emit_keyboard_close (GxrContext *self);

void
gxr_context_emit_quit (GxrContext *self, gpointer event);

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
