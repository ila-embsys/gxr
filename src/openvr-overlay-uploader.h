/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_OVERLAY_UPLOADER_H_
#define OPENVR_OVERLAY_UPLOADER_H_

#include <stdint.h>
#include <glib-object.h>

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#include <openvr_capi.h>

#include <gulkan-client.h>
#include <gulkan-texture.h>

#include "openvr-overlay.h"
#include "openvr-system.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_OVERLAY_UPLOADER openvr_overlay_uploader_get_type()
G_DECLARE_FINAL_TYPE (OpenVROverlayUploader, openvr_overlay_uploader,
                      OPENVR, OVERLAY_UPLOADER, GulkanClient)

struct _OpenVROverlayUploader
{
  GulkanClient parent_type;
};

struct _OpenVROverlayUploaderClass
{
  GObjectClass parent_class;
};

OpenVROverlayUploader *openvr_overlay_uploader_new (void);

bool
openvr_overlay_uploader_init_vulkan (OpenVROverlayUploader *self,
                                    bool enable_validation);

void
openvr_overlay_uploader_submit_frame (OpenVROverlayUploader *self,
                                     OpenVROverlay        *overlay,
                                     GulkanTexture  *texture);

G_END_DECLS

#endif /* OPENVR_OVERLAY_UPLOADER_H_ */
