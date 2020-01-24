/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openxr-context.h"

#include <gdk/gdk.h>

#include <glib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdarg.h>
#include <graphene.h>
#include <gulkan.h>

#include <openxr/openxr_reflection.h>

#include "gxr-types.h"
#include "gxr-context-private.h"

#include "openxr-overlay.h"
#include "openxr-action.h"

#include "gxr-io.h"
#include "gxr-manifest.h"

#define NUM_CONTROLLERS 2

struct _OpenXRContext
{
  GxrContext parent;

  XrInstance instance;
  XrSession session;
  XrSpace local_space;
  XrSpace view_space;
  XrSystemId system_id;
  XrViewConfigurationType view_config_type;

  XrSwapchain* swapchains;
  XrCompositionLayerProjectionView* projection_views;
  XrViewConfigurationView* configuration_views;

  XrGraphicsBindingVulkanKHR graphics_binding;

  XrSwapchainImageVulkanKHR** images;

  uint32_t view_count;

  bool is_visible;
  bool is_runnting;

  XrCompositionLayerProjection projection_layer;
  XrFrameState frame_state;
  XrView* views;

  int64_t swapchain_format;

  XrPath handPaths[NUM_CONTROLLERS];
  XrSpace handSpaces[NUM_CONTROLLERS];

  GSList *manifests;
};

G_DEFINE_TYPE (OpenXRContext, openxr_context, GXR_TYPE_CONTEXT)

static void
openxr_context_finalize (GObject *gobject);

static void
openxr_context_init (OpenXRContext *self)
{
  self->view_count = 0;
  self->views = NULL;
  self->manifests = NULL;
}

OpenXRContext *
openxr_context_new (void)
{
  return (OpenXRContext*) g_object_new (OPENXR_TYPE_CONTEXT, 0);
}

static const char* viewport_config_name = "/viewport_configuration/vr";

static const char*
xr_result_to_string(XrResult result)
{
  switch (result) {

#define MAKE_CASE(VAL, _)                                                      \
  case VAL: return #VAL;

    XR_LIST_ENUM_XrResult(MAKE_CASE)
  default: return "UNKNOWN";
  }
}

static bool
_check_xr_result (XrResult result, const char* format, ...)
{
  if (XR_SUCCEEDED(result))
    return true;

  const char * resultString = xr_result_to_string(result);

  char formatRes[XR_MAX_RESULT_STRING_SIZE]; // + " []\n"
  sprintf(formatRes, "%s [%s]\n", format, resultString);

  va_list args;
  va_start(args, format);
  vprintf(formatRes, args);
  va_end(args);
  return false;
}

static bool
_is_extension_supported (char* name, XrExtensionProperties* props, uint32_t count)
{
  for (uint32_t i = 0; i < count; i++)
    if (!strcmp(name, props[i].extensionName))
      return true;
  return false;
}

static bool
_check_vk_extension()
{
  XrResult result;
  uint32_t instanceExtensionCount = 0;
  result = xrEnumerateInstanceExtensionProperties(
    NULL, 0, &instanceExtensionCount, NULL);

  if (!_check_xr_result (result,
                 "Failed to enumerate number of instance extension properties"))
    return false;

  XrExtensionProperties instanceExtensionProperties[instanceExtensionCount];
  for (uint16_t i = 0; i < instanceExtensionCount; i++)
    instanceExtensionProperties[i] = (XrExtensionProperties){
      .type = XR_TYPE_EXTENSION_PROPERTIES,
    };

  result = xrEnumerateInstanceExtensionProperties(NULL, instanceExtensionCount,
                                                  &instanceExtensionCount,
                                                  instanceExtensionProperties);
  if (!_check_xr_result (result, "Failed to enumerate extension properties"))
    return false;

  result =
    _is_extension_supported (XR_KHR_VULKAN_ENABLE_EXTENSION_NAME,
                           instanceExtensionProperties, instanceExtensionCount);
  if (!_check_xr_result
      (result,
                 "Runtime does not support required instance extension %s\n",
                 XR_KHR_VULKAN_ENABLE_EXTENSION_NAME))
    return false;

  return true;
}

static bool
_enumerate_api_layers()
{
  uint32_t apiLayerCount;
  xrEnumerateApiLayerProperties(0, &apiLayerCount, NULL);

  XrApiLayerProperties apiLayerProperties[apiLayerCount];
  memset(apiLayerProperties, 0, apiLayerCount * sizeof(XrApiLayerProperties));

  for (uint32_t i = 0; i < apiLayerCount; i++) {
    apiLayerProperties[i].type = XR_TYPE_API_LAYER_PROPERTIES;
  }
  xrEnumerateApiLayerProperties(apiLayerCount, &apiLayerCount,
                                apiLayerProperties);

  for (uint32_t i = 0; i < apiLayerCount; i++) {
    if (strcmp(apiLayerProperties->layerName, "XR_APILAYER_LUNARG_api_dump") ==
        0) {
      g_print("XR_APILAYER_LUNARG_api_dump supported.\n");
    } else if (strcmp(apiLayerProperties->layerName,
                      "XR_APILAYER_LUNARG_core_validation") == 0) {
      g_print("XR_APILAYER_LUNARG_core_validation supported.\n");
    }
  }

  return true;
}

static bool
_create_instance(OpenXRContext* self)
{
  const char* const enabledExtensions[] = {
    XR_KHR_VULKAN_ENABLE_EXTENSION_NAME
  };

  XrInstanceCreateInfo instanceCreateInfo = {
    .type = XR_TYPE_INSTANCE_CREATE_INFO,
    .createFlags = 0,
    .enabledExtensionCount = 1,
    .enabledExtensionNames = enabledExtensions,
    .enabledApiLayerCount = 0,
    .applicationInfo = {
      .applicationName = "xrgears",
      .engineName = "xrgears",
      .applicationVersion = 1,
      .engineVersion = 1,
      .apiVersion = XR_CURRENT_API_VERSION,
    },
  };

  XrResult result;
  result = xrCreateInstance(&instanceCreateInfo, &self->instance);
  if (!_check_xr_result (result, "Failed to create XR instance."))
    return false;

  return true;
}

static bool
_create_system(OpenXRContext* self)
{
  XrPath vrConfigName;
  XrResult result;
  result = xrStringToPath(self->instance, viewport_config_name, &vrConfigName);
  _check_xr_result (result, "failed to get viewport configuration name");

  g_print("Got vrconfig %lu\n", vrConfigName);

  XrSystemGetInfo systemGetInfo = {
    .type = XR_TYPE_SYSTEM_GET_INFO,
    .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
  };

  result = xrGetSystem(self->instance, &systemGetInfo, &self->system_id);
  if (!_check_xr_result
      (result, "Failed to get system for %s viewport configuration.",
                 viewport_config_name))
    return false;

  XrSystemProperties systemProperties = {
    .type = XR_TYPE_SYSTEM_PROPERTIES,
    .graphicsProperties = { 0 },
    .trackingProperties = { 0 },
  };

  result =
    xrGetSystemProperties(self->instance, self->system_id, &systemProperties);
  if (!_check_xr_result (result, "Failed to get System properties"))
    return false;

  return true;
}

static bool
_set_up_views(OpenXRContext* self)
{
  uint32_t viewConfigurationCount;
  XrResult result;
  result = xrEnumerateViewConfigurations(self->instance, self->system_id, 0,
                                         &viewConfigurationCount, NULL);
  if (!_check_xr_result (result, "Failed to get view configuration count"))
    return false;

  XrViewConfigurationType viewConfigurations[viewConfigurationCount];
  result = xrEnumerateViewConfigurations(
    self->instance, self->system_id, viewConfigurationCount,
    &viewConfigurationCount, viewConfigurations);
  if (!_check_xr_result (result, "Failed to enumerate view configurations!"))
    return false;

  self->view_config_type = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
  XrViewConfigurationType optionalSecondaryViewConfigType =
    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;

  /* if struct (more specifically .type) is still 0 after searching, then
   we have not found the config. This way we don't need to set a bool
   found to true. */
  XrViewConfigurationProperties requiredViewConfigProperties = { 0 };
  XrViewConfigurationProperties secondaryViewConfigProperties = { 0 };

  for (uint32_t i = 0; i < viewConfigurationCount; ++i) {
    XrViewConfigurationProperties properties = {
      .type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES,
    };

    result = xrGetViewConfigurationProperties(
      self->instance, self->system_id, viewConfigurations[i], &properties);
    if (!_check_xr_result
	  (result, "Failed to get view configuration info %d!", i))
      return false;

    if (viewConfigurations[i] == self->view_config_type &&
        properties.viewConfigurationType == self->view_config_type) {
      requiredViewConfigProperties = properties;
    } else if (viewConfigurations[i] == optionalSecondaryViewConfigType &&
               properties.viewConfigurationType ==
                 optionalSecondaryViewConfigType) {
      secondaryViewConfigProperties = properties;
    }
  }
  if (requiredViewConfigProperties.type !=
      XR_TYPE_VIEW_CONFIGURATION_PROPERTIES) {
    g_print("Couldn't get required VR View Configuration %s from Runtime!\n",
              viewport_config_name);
    return false;
  }

  result = xrEnumerateViewConfigurationViews(self->instance, self->system_id,
                                             self->view_config_type, 0,
                                             &self->view_count, NULL);
  if (!_check_xr_result
      (result, "Failed to get view configuration view count!"))
    return false;

  self->configuration_views =
    malloc(sizeof(XrViewConfigurationView) * self->view_count);

  result = xrEnumerateViewConfigurationViews(
    self->instance, self->system_id, self->view_config_type, self->view_count,
    &self->view_count, self->configuration_views);
  if (!_check_xr_result
      (result, "Failed to enumerate view configuration views!"))
    return false;

  uint32_t secondaryViewConfigurationViewCount = 0;
  if (secondaryViewConfigProperties.type ==
      XR_TYPE_VIEW_CONFIGURATION_PROPERTIES) {

    result = xrEnumerateViewConfigurationViews(
      self->instance, self->system_id, optionalSecondaryViewConfigType, 0,
      &secondaryViewConfigurationViewCount, NULL);
    if (!_check_xr_result
	  (result, "Failed to get view configuration view count!"))
      return false;
  }

  if (secondaryViewConfigProperties.type ==
      XR_TYPE_VIEW_CONFIGURATION_PROPERTIES) {
    result = xrEnumerateViewConfigurationViews(
      self->instance, self->system_id, optionalSecondaryViewConfigType,
      secondaryViewConfigurationViewCount, &secondaryViewConfigurationViewCount,
      self->configuration_views);
    if (!_check_xr_result
	  (result, "Failed to enumerate view configuration views!"))
      return false;
  }

  return true;
}

static bool
_check_graphics_api_support(OpenXRContext* self)
{
  XrGraphicsRequirementsVulkanKHR vk_reqs = {
    .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR,
  };
  PFN_xrGetVulkanGraphicsRequirementsKHR GetVulkanGraphicsRequirements = NULL;
  XrResult result = xrGetInstanceProcAddr(
    self->instance, "xrGetVulkanGraphicsRequirementsKHR",
    (PFN_xrVoidFunction*)(&GetVulkanGraphicsRequirements));
  if (!_check_xr_result
      (result, "Failed to retrieve OpenXR Vulkan function pointer!"))
    return false;

  result =
    GetVulkanGraphicsRequirements(self->instance, self->system_id, &vk_reqs);
  if (!_check_xr_result
      (result, "Failed to get Vulkan graphics requirements!"))
    return false;

  XrVersion desired_version = XR_MAKE_VERSION(1, 0, 0);
  if (desired_version > vk_reqs.maxApiVersionSupported ||
      desired_version < vk_reqs.minApiVersionSupported) {
    g_printerr("Runtime does not support requested Vulkan version.\n");
    g_printerr("desired_version %lu\n", desired_version);
    g_printerr("minApiVersionSupported %lu\n", vk_reqs.minApiVersionSupported);
    g_printerr("maxApiVersionSupported %lu\n", vk_reqs.maxApiVersionSupported);
    return false;
  }
  return true;
}

static bool
_create_session(OpenXRContext* self)
{
  XrSessionCreateInfo session_create_info = {
    .type = XR_TYPE_SESSION_CREATE_INFO,
    .next = &self->graphics_binding,
    .systemId = self->system_id,
  };

  XrResult result =
    xrCreateSession(self->instance, &session_create_info, &self->session);
  if (!_check_xr_result (result, "Failed to create session"))
    return false;
  return true;
}

static bool
_is_space_supported (XrReferenceSpaceType *spaces,
                     uint32_t count,
                     XrReferenceSpaceType type)
{
  for (uint32_t i = 0; i < count; i++)
    if (spaces[i] == type)
      return true;
  return false;
}

static bool
_check_supported_spaces (OpenXRContext* self)
{
  uint32_t count;
  XrResult result = xrEnumerateReferenceSpaces (self->session, 0, &count, NULL);
  if (!_check_xr_result
      (result, "Getting number of reference spaces failed!"))
    return false;

  XrReferenceSpaceType spaces[count];
  result = xrEnumerateReferenceSpaces (self->session, count, &count, spaces);
  if (!_check_xr_result (result, "Enumerating reference spaces failed!"))
    return false;

  if (!_is_space_supported (spaces, count, XR_REFERENCE_SPACE_TYPE_LOCAL)) {
    g_print ("XR_REFERENCE_SPACE_TYPE_LOCAL unsupported.\n");
    return false;
  }

  if (!_is_space_supported (spaces, count, XR_REFERENCE_SPACE_TYPE_VIEW)) {
    g_print ("XR_REFERENCE_SPACE_TYPE_VIEW unsupported.\n");
    return false;
  }

  XrPosef identity = {
    .orientation = { .x = 0, .y = 0, .z = 0, .w = 1.0 },
    .position = { .x = 0, .y = 0, .z = 0 },
  };

  XrReferenceSpaceCreateInfo info = {
    .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
    .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL,
    .poseInReferenceSpace = identity,
  };
  result = xrCreateReferenceSpace(self->session, &info, &self->local_space);
  if (!_check_xr_result (result, "Failed to create local space."))
    return false;

  info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
  result = xrCreateReferenceSpace(self->session, &info, &self->view_space);
  if (!_check_xr_result (result, "Failed to create view space."))
    return false;

  return true;
}

static bool
_begin_session(OpenXRContext* self)
{
  XrSessionBeginInfo sessionBeginInfo = {
    .type = XR_TYPE_SESSION_BEGIN_INFO,
    .primaryViewConfigurationType = self->view_config_type,
  };
  XrResult result = xrBeginSession(self->session, &sessionBeginInfo);
  if (!_check_xr_result (result, "Failed to begin session!"))
    return false;

  return true;
}

static bool
_create_swapchains(OpenXRContext* self)
{
  XrResult result;
  uint32_t swapchainFormatCount;
  result =
    xrEnumerateSwapchainFormats(self->session, 0, &swapchainFormatCount, NULL);
  if (!_check_xr_result
      (result, "Failed to get number of supported swapchain formats"))
    return false;

  int64_t swapchainFormats[swapchainFormatCount];
  result = xrEnumerateSwapchainFormats(self->session, swapchainFormatCount,
                                       &swapchainFormatCount, swapchainFormats);
  if (!_check_xr_result (result, "Failed to enumerate swapchain formats"))
    return false;

  /* First create swapchains and query the length for each swapchain. */
  self->swapchains = malloc(sizeof(XrSwapchain) * self->view_count);

  uint32_t swapchainLength[self->view_count];

  self->swapchain_format = swapchainFormats[0];

  for (uint32_t i = 0; i < self->view_count; i++) {
    XrSwapchainCreateInfo swapchainCreateInfo = {
      .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
      .usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
                    XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
      .createFlags = 0,
      // just use the first enumerated format
      .format = swapchainFormats[0],
      .sampleCount = 1,
      .width = self->configuration_views[i].recommendedImageRectWidth,
      .height = self->configuration_views[i].recommendedImageRectHeight,
      .faceCount = 1,
      .arraySize = 1,
      .mipCount = 1,
    };

    g_print("Swapchain %d dimensions: %dx%d\n", i,
            self->configuration_views[i].recommendedImageRectWidth,
            self->configuration_views[i].recommendedImageRectHeight);

    result = xrCreateSwapchain(self->session, &swapchainCreateInfo,
                               &self->swapchains[i]);
    if (!_check_xr_result (result, "Failed to create swapchain %d!", i))
      return false;

    result = xrEnumerateSwapchainImages(self->swapchains[i], 0,
                                        &swapchainLength[i], NULL);
    if (!_check_xr_result (result, "Failed to enumerate swapchains"))
      return false;
  }

  // most likely all swapchains have the same length, but let's not fail
  // if they are not
  uint32_t maxSwapchainLength = 0;
  for (uint32_t i = 0; i < self->view_count; i++) {
    if (swapchainLength[i] > maxSwapchainLength) {
      maxSwapchainLength = swapchainLength[i];
    }
  }

  self->images = malloc(sizeof(XrSwapchainImageVulkanKHR*) * self->view_count);
  for (uint32_t i = 0; i < self->view_count; i++) {
    self->images[i] =
      malloc(sizeof(XrSwapchainImageVulkanKHR) * maxSwapchainLength);
  }


  for (uint32_t i = 0; i < self->view_count; i++) {
    result = xrEnumerateSwapchainImages(
      self->swapchains[i], swapchainLength[i], &swapchainLength[i],
      (XrSwapchainImageBaseHeader*)self->images[i]);
    if (!_check_xr_result (result, "Failed to enumerate swapchains"))
      return false;
  }

  return true;
}

static void
_create_projection_views(OpenXRContext* self)
{
  self->projection_views =
    malloc(sizeof(XrCompositionLayerProjectionView) * self->view_count);

  for (uint32_t i = 0; i < self->view_count; i++)
    self->projection_views[i] = (XrCompositionLayerProjectionView) {
      .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
      .subImage = {
        .swapchain = self->swapchains[i],
        .imageRect = {
          .extent = {
              .width = (int32_t) self->configuration_views[i].recommendedImageRectWidth,
              .height = (int32_t) self->configuration_views[i].recommendedImageRectHeight,
          },
        },
      },
    };
}

bool
openxr_context_begin_frame(OpenXRContext* self)
{
  XrResult result;

  self->frame_state = (XrFrameState){
    .type = XR_TYPE_FRAME_STATE,
  };
  XrFrameWaitInfo frameWaitInfo = {
    .type = XR_TYPE_FRAME_WAIT_INFO,
  };
  result = xrWaitFrame(self->session, &frameWaitInfo, &self->frame_state);
  if (!_check_xr_result
      (result, "xrWaitFrame() was not successful, exiting..."))
    return false;

  if (!self->is_visible)
    return false;

  // --- Create projection matrices and view matrices for each eye
  XrViewLocateInfo viewLocateInfo = {
    .type = XR_TYPE_VIEW_LOCATE_INFO,
    .displayTime = self->frame_state.predictedDisplayTime,
    .space = self->local_space,
  };

  self->views = malloc(sizeof(XrView) * self->view_count);
  for (uint32_t i = 0; i < self->view_count; i++) {
    self->views[i].type = XR_TYPE_VIEW;
  }

  XrViewState viewState = {
    .type = XR_TYPE_VIEW_STATE,
  };
  uint32_t viewCountOutput;
  result = xrLocateViews(self->session, &viewLocateInfo, &viewState,
                         self->view_count, &viewCountOutput, self->views);
  if (!_check_xr_result (result, "Could not locate views"))
    return false;

  // --- Begin frame
  XrFrameBeginInfo frameBeginInfo = {
    .type = XR_TYPE_FRAME_BEGIN_INFO,
  };

  result = xrBeginFrame(self->session, &frameBeginInfo);
  if (!_check_xr_result (result, "failed to begin frame!"))
    return false;

  return true;
}

bool
openxr_context_aquire_swapchain(OpenXRContext* self,
                                uint32_t i,
                                uint32_t* buffer_index)
{
  XrResult result;

  XrSwapchainImageAcquireInfo swapchainImageAcquireInfo = {
    .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO,
  };

  result = xrAcquireSwapchainImage(self->swapchains[i],
                                   &swapchainImageAcquireInfo, buffer_index);
  if (!_check_xr_result (result, "failed to acquire swapchain image!"))
    return false;

  XrSwapchainImageWaitInfo swapchainImageWaitInfo = {
    .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
    .timeout = INT64_MAX,
  };
  result = xrWaitSwapchainImage(self->swapchains[i], &swapchainImageWaitInfo);
  if (!_check_xr_result (result, "failed to wait for swapchain image!"))
    return false;

  self->projection_views[i].pose = self->views[i].pose;
  self->projection_views[i].fov = self->views[i].fov;
  self->projection_views[i].subImage.imageArrayIndex = *buffer_index;

  return true;
}

bool
openxr_context_release_swapchain(OpenXRContext* self,
                                 uint32_t eye)
{
  XrSwapchainImageReleaseInfo swapchainImageReleaseInfo = {
    .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
  };
  XrResult result =
    xrReleaseSwapchainImage(self->swapchains[eye], &swapchainImageReleaseInfo);
  if (!_check_xr_result (result, "failed to release swapchain image!"))
    return false;

  return true;
}

bool
openxr_context_end_frame(OpenXRContext* self)
{
  XrResult result;

  const XrCompositionLayerBaseHeader* const projection_layers[1] = {
    (const XrCompositionLayerBaseHeader* const) & self->projection_layer
  };
  XrFrameEndInfo frame_end_info = {
    .type = XR_TYPE_FRAME_END_INFO,
    .displayTime = self->frame_state.predictedDisplayTime,
    .layerCount = 1,
    .layers = projection_layers,
    .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
  };
  result = xrEndFrame(self->session, &frame_end_info);
  if (!_check_xr_result (result, "failed to end frame!"))
    return false;

  free(self->views);
  self->views = NULL;

  return true;
}

void
openxr_context_cleanup(OpenXRContext* self)
{
  if (!self->swapchains)
    {
      for (uint32_t i = 0; i < self->view_count; i++) {
        xrDestroySwapchain(self->swapchains[i]);
      }
    }

  if (self->swapchains)
    free(self->swapchains);
  if (self->local_space)
    xrDestroySpace(self->local_space);
  if (self->session)
    xrDestroySession(self->session);
  if (self->instance)
    xrDestroyInstance(self->instance);
}

static void
openxr_context_finalize (GObject *gobject)
{
  OpenXRContext *self = OPENXR_CONTEXT (gobject);
  openxr_context_cleanup (self);
  g_slist_free_full (self->manifests, g_object_unref);
  G_OBJECT_CLASS (openxr_context_parent_class)->finalize (gobject);
}

XrSwapchainImageVulkanKHR**
openxr_context_get_images(OpenXRContext *self)
{
  return self->images;
}

void
openxr_context_get_swapchain_dimensions (OpenXRContext *self,
                                         uint32_t i,
                                         uint32_t *width,
                                         uint32_t *height)
{
  *width = self->configuration_views[i].recommendedImageRectWidth;
  *height = self->configuration_views[i].recommendedImageRectHeight;
}

static void
_get_projection_matrix_from_fov (const XrFovf fov,
                                 const float near_z,
                                 const float far_z,
                                 graphene_matrix_t *mat)
{
  const float tan_left = tanf(fov.angleLeft);
  const float tan_right = tanf(fov.angleRight);

  const float tan_down = tanf(fov.angleDown);
  const float tan_up = tanf(fov.angleUp);

  const float tan_width = tan_right - tan_left;
  const float tan_height = tan_up - tan_down;

  const float a11 = 2 / tan_width;
  const float a22 = 2 / tan_height;

  const float a31 = (tan_right + tan_left) / tan_width;
  const float a32 = (tan_up + tan_down) / tan_height;
  const float a33 = -far_z / (far_z - near_z);

  const float a43 = -(far_z * near_z) / (far_z - near_z);

  const float m[16] = {
      a11, 0, 0, 0, 0, a22, 0, 0, a31, a32, a33, -1, 0, 0, a43, 0,
  };

  graphene_matrix_init_from_float (mat, m);
}

static void
_get_view_matrix_from_pose (XrPosef *pose,
                            graphene_matrix_t *mat)
{
  graphene_quaternion_t q;
  graphene_quaternion_init (&q,
                            pose->orientation.x, pose->orientation.y,
                            pose->orientation.z, pose->orientation.w);

  graphene_matrix_t rotation;
  graphene_matrix_init_identity (&rotation);
  graphene_matrix_rotate_quaternion (&rotation, &q);

  graphene_point3d_t position = { pose->position.x,
                                  pose->position.y,
                                  pose->position.z };

  graphene_matrix_t translation;
  graphene_matrix_init_translate (&translation, &position);

  graphene_matrix_t view;
  graphene_matrix_multiply (&rotation, &translation, &view);

  graphene_matrix_inverse (&view, mat);
}

void
openxr_context_get_position (OpenXRContext *self,
                             uint32_t i,
                             graphene_vec4_t *v)
{
  graphene_vec4_init (v,
                      self->views[i].pose.position.x,
                      self->views[i].pose.position.y,
                      self->views[i].pose.position.z,
                      1.0f);
}

static bool
_space_location_valid (XrSpaceLocation *sl)
{
  return (sl->locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)    != 0 &&
         (sl->locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0;
}

static gboolean
_get_head_pose (GxrContext *context, graphene_matrix_t *pose)
{
  OpenXRContext *self = OPENXR_CONTEXT (context);

  XrSpaceLocation space_location = {
    .type = XR_TYPE_SPACE_LOCATION,
    .next = NULL
  };

  XrResult result = xrLocateSpace (self->view_space, self->local_space,
                                   self->frame_state.predictedDisplayTime,
                                  &space_location);
  _check_xr_result (result, "Failed to locate head space.");

  bool valid = _space_location_valid (&space_location);
  if (!valid)
    {
      g_printerr ("Could not get valid head pose.\n");
      graphene_matrix_init_identity (pose);
      return FALSE;
    }

  _get_view_matrix_from_pose(&space_location.pose, pose);
  return TRUE;
}

VkFormat
openxr_context_get_swapchain_format (OpenXRContext *self)
{
  return (VkFormat) self->swapchain_format;
}

static void
_get_render_dimensions (GxrContext *context,
                        uint32_t   *width,
                        uint32_t   *height)
{
  OpenXRContext *self = OPENXR_CONTEXT (context);
  openxr_context_get_swapchain_dimensions (self, 0, width, height);
}

static gboolean
_is_input_available ()
{
  return TRUE;
}

static void
_get_frustum_angles (GxrContext *context, GxrEye eye,
                     float *left, float *right,
                     float *top, float *bottom)
{
  (void) context;
  (void) eye;
  *left = 1; *right = 1; *top = 1; *bottom = 1;
  g_warning ("_get_frustum_angles not implemented in OpenXR.\n");
}

static gboolean
_init_runtime (GxrContext *context,
               GxrAppType type)
{
  OpenXRContext *self = OPENXR_CONTEXT (context);
  switch (type)
  {
    case GXR_APP_SCENE:
      break;
    case GXR_APP_OVERLAY:
      g_print ("stub: Overlay app type is not implemented in OpenXR.\n");
      break;
    case GXR_APP_HEADLESS:
      g_print ("stub: Headless app type is not implemented in OpenXR.\n");
      break;
    default:
      g_error ("Unknown app type %d\n", type);
  }

  if (!_check_vk_extension())
    return false;

  if (!_enumerate_api_layers())
    return false;

  if (!_create_instance(self))
    return false;

  if (!_create_system(self))
    return false;

  if (type != GXR_APP_HEADLESS)
    {
      if (!_set_up_views(self))
        return false;

      if (!_check_graphics_api_support(self))
        return false;
    }

  return true;
}

static gboolean
_init_session (GxrContext *context)
{
  OpenXRContext *self = OPENXR_CONTEXT (context);

  GulkanClient *gc = gxr_context_get_gulkan (context);
  GulkanDevice *gk_device = gulkan_client_get_device (gc);
  uint32_t queue_family_index =
    gulkan_device_get_queue_family_index (gk_device);

  self->graphics_binding = (XrGraphicsBindingVulkanKHR){
    .type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR,
    .instance = gulkan_client_get_instance_handle (gc),
    .physicalDevice = gulkan_device_get_physical_handle (gk_device),
    .device = gulkan_device_get_handle (gk_device),
    .queueFamilyIndex = queue_family_index,
    .queueIndex = 0,
  };


  if (!_create_session(self))
    return false;

  if (!_check_supported_spaces(self))
    return false;

  if (!_begin_session(self))
    return false;

  if (!_create_swapchains(self))
    return false;

  g_print("Created swapchains.\n");

  _create_projection_views(self);

  self->is_visible = true;
  self->is_runnting = true;

  self->projection_layer = (XrCompositionLayerProjection){
    .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
    .layerFlags = 0,
    .space = self->local_space,
    .viewCount = self->view_count,
    .views = self->projection_views,
  };

  return true;
}

static void
_poll_event (GxrContext *context)
{
  OpenXRContext * self = OPENXR_CONTEXT (context);

  XrEventDataBuffer runtimeEvent = {
    .type = XR_TYPE_EVENT_DATA_BUFFER,
    .next = NULL,
  };

  XrResult pollResult = xrPollEvent (self->instance, &runtimeEvent);

  while (pollResult == XR_SUCCESS)
    {
      switch (runtimeEvent.type)
      {
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
        {
          XrEventDataSessionStateChanged* event =
            (XrEventDataSessionStateChanged*)&runtimeEvent;
          XrSessionState state = event->state;
          self->is_visible = event->state <= XR_SESSION_STATE_FOCUSED;
          g_debug ("EVENT: session state changed to %d. Visible: %d\n",
                  state, self->is_visible);
          if (event->state >= XR_SESSION_STATE_STOPPING)
            {
              self->is_runnting = false;

              GxrQuitEvent *quit_event = g_malloc (sizeof (GxrQuitEvent));
              quit_event->reason = GXR_QUIT_SHUTDOWN;
              g_debug ("Event: sending VR_QUIT_SHUTDOWN signal\n");
              gxr_context_emit_quit (context, quit_event);

              xrEndSession (self->session);

              /* don't poll further events, leave them to "next" session */
              return;
            }
          break;
        }
        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
        {
          GxrQuitEvent *event = g_malloc (sizeof (GxrQuitEvent));
          event->reason = GXR_QUIT_SHUTDOWN;
          g_debug ("Event: sending VR_QUIT_SHUTDOWN signal\n");
          gxr_context_emit_quit (context, event);
          break;
        }
        case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
        {
          g_print ("Event: STUB: interaction profile changed\n");
          break;
        }
        case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
        {
          g_print ("Event: STUB: reference space change pending\n");
          break;
        }
        default:
        {
          char buffer[XR_MAX_STRUCTURE_NAME_SIZE];
          xrStructureTypeToString (self->instance, runtimeEvent.type, buffer);
          g_print ("Event: Unhandled event type %s (%d)\n",
                   buffer, runtimeEvent.type);
          break;
        }
      }

      pollResult = xrPollEvent (self->instance, &runtimeEvent);
    }

  if (pollResult == XR_EVENT_UNAVAILABLE)
    {
      // this is the usual case
      // g_debug ("Poll events: No more events in queue\n");
    }
  else
    {
      g_printerr ("Failed to poll events!\n");
      return;
    }
}

static void
_show_keyboard (GxrContext *context)
{
  (void) context;
  g_print ("Stub: show OpenXR keyboard\n");
}

XrInstance
openxr_context_get_openxr_instance (OpenXRContext *self)
{
  return self->instance;
}

XrSession
openxr_context_get_openxr_session (OpenXRContext *self)
{
  return self->session;
}

XrSpace
openxr_context_get_tracked_space (OpenXRContext *self)
{
  return self->local_space;
}

static gboolean
_init_framebuffers (GxrContext           *context,
                    VkExtent2D            extent,
                    VkSampleCountFlagBits sample_count,
                    GulkanFrameBuffer    *framebuffers[2],
                    GulkanRenderPass    **render_pass)
{
  OpenXRContext *self = OPENXR_CONTEXT (context);
  XrSwapchainImageVulkanKHR** images = openxr_context_get_images(self);
  VkFormat format = openxr_context_get_swapchain_format(self);

  GulkanClient *gc = gxr_context_get_gulkan (context);
  GulkanDevice *device = gulkan_client_get_device (gc);

  *render_pass =
    gulkan_render_pass_new (device, sample_count, format,
                            VK_IMAGE_LAYOUT_UNDEFINED, TRUE);
  if (!*render_pass)
    {
      g_printerr ("Could not init render pass.\n");
      return FALSE;
    }

  for (uint32_t eye = 0; eye < 2; eye++)
    {
      framebuffers[eye] =
        gulkan_frame_buffer_new_from_image_with_depth (device, *render_pass,
                                                       images[eye][0].image,
                                                       extent, sample_count,
                                                       format);
      if (!framebuffers[eye])
        {
          g_printerr("Could not initialize frambuffer.");
          return FALSE;
        }
  }
  return TRUE;
}

/* Not required in OpenXR */
static gboolean
_submit_framebuffers (GxrContext           *self,
                      GulkanFrameBuffer    *framebuffers[2],
                      VkExtent2D            extent,
                      VkSampleCountFlagBits sample_count)
{
  (void) self;
  (void) framebuffers;
  (void) extent;
  (void) sample_count;
  return TRUE;
}

/*
 * Dummy values to make the pipeline initialize,
 * TODO: Add GLTF models for OpenXR backend.
 */
static uint32_t
_get_model_vertex_stride (GxrContext *self)
{
  (void) self;
  return 1;
}

static uint32_t
_get_model_normal_offset (GxrContext *self)
{
  (void) self;
  return 1;
}

static uint32_t
_get_model_uv_offset (GxrContext *self)
{
  (void) self;
  return 2;
}

static void
_get_projection (GxrContext *context,
                 GxrEye eye,
                 float near,
                 float far,
                 graphene_matrix_t *mat)
{
  OpenXRContext *self = OPENXR_CONTEXT (context);
  if (self->views == NULL)
    {
      g_warning ("get_projection needs to be called "
                 "between begin and end frame.\n");
      graphene_matrix_init_identity (mat);
      return;
    }
  _get_projection_matrix_from_fov (self->views[eye].fov, near, far, mat);
}

static void
_get_view (GxrContext *context,
           GxrEye eye,
           graphene_matrix_t *mat)
{
  OpenXRContext *self = OPENXR_CONTEXT (context);
  if (self->views == NULL)
    {
      g_warning ("get_view needs to be called between begin and end frame.\n");
      graphene_matrix_init_identity (mat);
      return;
    }
  _get_view_matrix_from_pose (&self->views[eye].pose, mat);
}

static gboolean
_begin_frame (GxrContext *context)
{
  OpenXRContext *self = OPENXR_CONTEXT (context);

  if (!openxr_context_begin_frame(self)) {
    g_printerr ("Could not begin XR frame.\n");
    return FALSE;
  }

  for (uint32_t i = 0; i < 2; i++) {
    uint32_t buffer_index;
    if (!openxr_context_aquire_swapchain (self, i, &buffer_index))
      {
        g_printerr ("Could not aquire xr swapchain\n");
        return FALSE;
      }
  }

  return TRUE;
}

static gboolean
_end_frame (GxrContext *context,
            GxrPose *poses)
{
  (void) poses;

  OpenXRContext *self = OPENXR_CONTEXT (context);
  for (uint32_t i = 0; i < 2; i++)
  if (!openxr_context_release_swapchain(self, i)) {
      g_printerr("Could not release xr swapchain\n");
      return FALSE;
  }

  if (!openxr_context_end_frame(self)) {
    g_printerr ("Could not end xr frame\n");
  }

  return TRUE;
}

static void
_acknowledge_quit (GxrContext *context)
{
  /* TODO: Implement in OpenXR */
  (void) context;
}

static gboolean
_is_tracked_device_connected (GxrContext *context, uint32_t i)
{
  /* TODO: Implement in OpenXR */
  (void) context;
  (void) i;
  return FALSE;
}

static gboolean
_device_is_controller (GxrContext *context, uint32_t i)
{
  /* TODO: Implement in OpenXR */
  (void) context;
  (void) i;
  return FALSE;
}

static gchar*
_get_device_model_name (GxrContext *context, uint32_t i)
{
  /* TODO: Implement in OpenXR */
  (void) context;
  (void) i;
  return (gchar*) g_malloc (1);
}

static gboolean
_load_model (GxrContext         *context,
             GulkanVertexBuffer *vbo,
             GulkanTexture     **texture,
             VkSampler          *sampler,
             const char         *model_name)
{
  /* TODO: Implement in OpenXR */
  (void) context;
  (void) vbo;
  (void) texture;
  (void) sampler;
  (void) model_name;
  return FALSE;
}

static gboolean
_is_another_scene_running (GxrContext *context)
{
  /* TODO: Implement in OpenXR */
  (void) context;
  return FALSE;
}

static void
_set_keyboard_transform (GxrContext        *context,
                         graphene_matrix_t *transform)
{
  /* TODO: Implement in OpenXR */
  (void) context;
  (void) transform;
}

static GSList *
_get_model_list (GxrContext *self)
{
  /* TODO: Implement in OpenXR */
  (void) self;
  return NULL;
}

static GxrActionSet *
_new_action_set_from_url (GxrContext *context, gchar *url)
{
  return (GxrActionSet*)
    openxr_action_set_new_from_url (OPENXR_CONTEXT (context), url);
}

static gboolean
_load_action_manifest (GxrContext *self,
                       const char *cache_name,
                       const char *resource_path,
                       const char *manifest_name,
                       const char *first_binding,
                       va_list     args)
{
  (void) self;
  (void) cache_name;

  GError *error;
  GString *actions_res_path = g_string_new ("");
  g_string_printf (actions_res_path, "%s/%s", resource_path, manifest_name);

  const char* current = first_binding;

  while (current != NULL)
    {
      /* stream can not be reset/reused, has to be recreated */
      GInputStream *actions_res_input_stream =
      g_resources_open_stream (actions_res_path->str,
                               G_RESOURCE_LOOKUP_FLAGS_NONE,
                               &error);

      GString *bindings_res_path = g_string_new ("");
      g_string_printf (bindings_res_path, "%s/%s", resource_path, current);
      GInputStream * bindings_res_input_stream =
      g_resources_open_stream (bindings_res_path->str,
                               G_RESOURCE_LOOKUP_FLAGS_NONE,
                               &error);


      g_debug ("Loading %s with %s\n", actions_res_path->str,
               bindings_res_path->str);
      GxrManifest *manifest = gxr_manifest_new ();
      gxr_manifest_load (manifest,
                         actions_res_input_stream, bindings_res_input_stream);

      g_debug ("Loaded manifest for interaction profile %s\n",
               gxr_manifest_get_interaction_profile (manifest));

      OpenXRContext *octx = OPENXR_CONTEXT (self);
      octx->manifests = g_slist_append (octx->manifests, manifest);

      current = va_arg (args, const char*);

      g_string_free (bindings_res_path, TRUE);
      g_object_unref (bindings_res_input_stream);
      g_object_unref (actions_res_input_stream);
    }

  va_end (args);

  g_string_free (actions_res_path, TRUE);

  g_debug ("Loaded action manifests\n");

  return TRUE;
}

static GxrAction *
_new_action_from_type_url (GxrContext   *self,
                           GxrActionSet *action_set,
                           GxrActionType type,
                           char          *url)
{
  return GXR_ACTION (openxr_action_new_from_type_url (OPENXR_CONTEXT (self),
                                                      action_set, type, url));
}

static GxrOverlay *
_new_overlay (GxrContext *self, gchar* key)
{
  (void) self;
  (void) key;
  return GXR_OVERLAY (openxr_overlay_new ());
}

GSList *
openxr_context_get_manifests (OpenXRContext *self)
{
  return self->manifests;
}

static void
_request_quit (GxrContext *context)
{
  OpenXRContext *self = OPENXR_CONTEXT (context);
  /* _poll_event will send quit event once session is in STOPPING state */
  xrRequestExitSession (self->session);
}

static bool
_get_instance_extensions (GxrContext *self, GSList **out_list)
{
  (void) self;
  (void) out_list;
  g_print ("Stub: get instance extensions\n");
  return TRUE;
}

static bool
_get_device_extensions (GxrContext *self, GulkanClient *gc, GSList **out_list)
{
  (void) self;
  (void) gc;
  (void) out_list;
  g_print ("Stub: get device extensions\n");
  return TRUE;
}

static void
openxr_context_class_init (OpenXRContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = openxr_context_finalize;

  GxrContextClass *gxr_context_class = GXR_CONTEXT_CLASS (klass);
  gxr_context_class->get_render_dimensions = _get_render_dimensions;
  gxr_context_class->is_input_available = _is_input_available;
  gxr_context_class->get_frustum_angles = _get_frustum_angles;
  gxr_context_class->get_head_pose = _get_head_pose;
  gxr_context_class->init_runtime = _init_runtime;
  gxr_context_class->init_session = _init_session;
  gxr_context_class->poll_event = _poll_event;
  gxr_context_class->show_keyboard = _show_keyboard;
  gxr_context_class->init_framebuffers = _init_framebuffers;
  gxr_context_class->get_model_vertex_stride = _get_model_vertex_stride;
  gxr_context_class->get_model_normal_offset = _get_model_normal_offset;
  gxr_context_class->get_model_uv_offset = _get_model_uv_offset;
  gxr_context_class->submit_framebuffers = _submit_framebuffers;
  gxr_context_class->get_projection = _get_projection;
  gxr_context_class->get_view = _get_view;
  gxr_context_class->begin_frame = _begin_frame;
  gxr_context_class->end_frame = _end_frame;
  gxr_context_class->acknowledge_quit = _acknowledge_quit;
  gxr_context_class->is_tracked_device_connected = _is_tracked_device_connected;
  gxr_context_class->device_is_controller = _device_is_controller;
  gxr_context_class->get_device_model_name = _get_device_model_name;
  gxr_context_class->load_model = _load_model;
  gxr_context_class->is_another_scene_running = _is_another_scene_running;
  gxr_context_class->set_keyboard_transform = _set_keyboard_transform;
  gxr_context_class->get_model_list = _get_model_list;
  gxr_context_class->new_action_set_from_url = _new_action_set_from_url;
  gxr_context_class->load_action_manifest = _load_action_manifest;
  gxr_context_class->new_action_from_type_url = _new_action_from_type_url;
  gxr_context_class->new_overlay = _new_overlay;
  gxr_context_class->request_quit = _request_quit;
  gxr_context_class->get_instance_extensions = _get_instance_extensions;
  gxr_context_class->get_device_extensions = _get_device_extensions;
}
