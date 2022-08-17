/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_CONTEXT_PRIVATE_H_
#define GXR_CONTEXT_PRIVATE_H_

#include "gxr-context.h"

#include "gxr-manifest.h"

XrInstance
gxr_context_get_openxr_instance (GxrContext *self);

XrSession
gxr_context_get_openxr_session (GxrContext *self);

XrSpace
gxr_context_get_tracked_space (GxrContext *self);

GxrManifest *
gxr_context_get_manifest (GxrContext *self);

XrTime
gxr_context_get_predicted_display_time (GxrContext *self);

XrSessionState
gxr_context_get_session_state (GxrContext *self);

#endif /* GXR_CONTEXT_PRIVATE_H_ */
