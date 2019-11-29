/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <gulkan.h>
#include "gxr.h"

#define ENUM_TO_STR(r) case r: return #r

static const gchar*
gxr_api_string (GxrApi v)
{
  switch (v)
    {
      ENUM_TO_STR(GXR_API_OPENXR);
      ENUM_TO_STR(GXR_API_OPENVR);
      default:
        return "UNKNOWN API";
    }
}

#ifdef GXR_HAS_OPENXR
static void
_init_openxr (OpenXRContext *context)
{
  GulkanClient *gc = gulkan_client_new ();
  g_assert (gulkan_client_init_vulkan (gc, NULL, NULL));

  GulkanInstance *gk_instance = gulkan_client_get_instance (gc);
  VkInstance vk_instance = gulkan_instance_get_handle (gk_instance);

  GulkanDevice *gk_device = gulkan_client_get_device (gc);
  VkDevice vk_device = gulkan_device_get_handle (gk_device);

  VkPhysicalDevice physical_device =
    gulkan_device_get_physical_handle (gk_device);

  uint32_t queue_family_index = gulkan_device_get_queue_family_index (gk_device);

  uint32_t queue_index = 0;

  g_assert (openxr_context_initialize (context, vk_instance,
                                       physical_device,
                                       vk_device,
                                       queue_family_index,
                                       queue_index));
}
#endif

#ifdef GXR_HAS_OPENVR
static void
_init_openvr (OpenVRContext *context)
{
  g_assert (openvr_context_initialize (context, GXR_APP_OVERLAY));
  g_assert (gxr_context_is_valid (GXR_CONTEXT (context)));
  g_object_unref (context);
}
#endif

static void
_test_minimal ()
{
  GxrContext *context = gxr_context_get_instance ();
  g_assert_nonnull (context);

  GxrApi api = gxr_context_get_api (context);
  g_print ("Using API: %s\n", gxr_api_string (api));

  switch (api)
    {
#ifdef GXR_HAS_OPENVR
      case GXR_API_OPENVR:
        g_print ("Initializing OpenVR context.");
        _init_openvr (OPENVR_CONTEXT (context));
        break;
#endif
#ifdef GXR_HAS_OPENXR
      case GXR_API_OPENXR:
        g_print ("Initializing OpenXR context.");
        _init_openxr (OPENXR_CONTEXT (context));
        break;
#endif
      default:
        g_printerr ("Unsupported XR API selected.\n");
    }

  g_object_unref (context);
}

int
main ()
{
  _test_minimal ();

  return 0;
}
