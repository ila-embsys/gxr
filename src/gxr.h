/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#ifndef GXR_H
#define GXR_H

#include <glib.h>

#define GXR_INSIDE

#include "gxr-config.h"
#include "gxr-version.h"
#include "gxr-enums.h"
#include "gxr-types.h"
#include "gxr-time.h"
#include "gxr-context.h"

#ifdef GXR_HAS_OPENVR
  #include "gxr-io.h"
  #include "openvr-action.h"
  #include "openvr-action-set.h"
  #include "openvr-compositor.h"
  #include "openvr-context.h"
  #include "openvr-overlay.h"
  #include "openvr-system.h"
  #include "openvr-model.h"
#endif

#ifdef GXR_HAS_OPENXR
  #include "openxr-context.h"
#endif

#undef GXR_INSIDE

#endif
