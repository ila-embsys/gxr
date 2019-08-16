/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_SYSTEM_PRIVATE_H_
#define GXR_SYSTEM_PRIVATE_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib.h>
#include "openvr-wrapper.h"
#include "gxr-enums.h"


gchar*
openvr_system_get_device_string (TrackedDeviceIndex_t device_index,
                                 ETrackedDeviceProperty property);

GxrEye
openvr_system_eye_to_gxr (EVREye eye);

EVREye
openvr_system_eye_to_openvr (GxrEye eye);

#endif /* GXR_SYSTEM_PRIVATE_H_ */
