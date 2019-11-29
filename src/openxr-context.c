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

#include <openxr/openxr_reflection.h>

#include "gxr-types.h"

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
  bool is_initialized;

  XrCompositionLayerProjection projection_layer;
  XrFrameState frame_state;
  XrView* views;

  int64_t swapchain_format;

  XrActionSet wm_actionset;
  XrAction grabAction;
  XrAction poseAction;
  XrPath handPaths[NUM_CONTROLLERS];
  XrSpace handSpaces[NUM_CONTROLLERS];
};

G_DEFINE_TYPE (OpenXRContext, openxr_context, GXR_TYPE_CONTEXT)

enum {
  DIGITAL_EVENT,
  ANALOG_EVENT,
  POSE_EVENT,
  LAST_SIGNAL
};

static guint action_signals[LAST_SIGNAL] = { 0 };

static void
openxr_context_finalize (GObject *gobject);

static void
openxr_context_init (OpenXRContext *self)
{
  self->is_initialized = false;
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

    XR_LIST_ENUM_XrResult(MAKE_CASE);
  default: return "UNKNOWN";
  }
}

bool
xr_result(XrResult result, const char* format, ...)
{
  if (XR_SUCCEEDED(result))
    return true;

  const char * resultString = xr_result_to_string(result);

  size_t len1 = strlen(format);
  size_t len2 = strlen(resultString) + 1;
  char formatRes[len1 + len2 + 4]; // + " []\n"
  sprintf(formatRes, "%s [%s]\n", format, resultString);

  va_list args;
  va_start(args, format);
  vprintf(formatRes, args);
  va_end(args);
  return false;
}

bool
is_extension_supported(char* name, XrExtensionProperties* props, uint32_t count)
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

  if (!xr_result(result,
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
  if (!xr_result(result, "Failed to enumerate extension properties"))
    return false;

  result =
    is_extension_supported(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME,
                           instanceExtensionProperties, instanceExtensionCount);
  if (!xr_result(result,
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
  if (!xr_result(result, "Failed to create XR instance."))
    return false;

  return true;
}

static bool
_create_system(OpenXRContext* self)
{
  XrPath vrConfigName;
  XrResult result;
  result = xrStringToPath(self->instance, viewport_config_name, &vrConfigName);
  xr_result(result, "failed to get viewport configuration name");

  g_print("Got vrconfig %lu\n", vrConfigName);

  XrSystemGetInfo systemGetInfo = {
    .type = XR_TYPE_SYSTEM_GET_INFO,
    .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
  };

  result = xrGetSystem(self->instance, &systemGetInfo, &self->system_id);
  if (!xr_result(result, "Failed to get system for %s viewport configuration.",
                 viewport_config_name))
    return false;

  XrSystemProperties systemProperties = {
    .type = XR_TYPE_SYSTEM_PROPERTIES,
    .graphicsProperties = { 0 },
    .trackingProperties = { 0 },
  };

  result =
    xrGetSystemProperties(self->instance, self->system_id, &systemProperties);
  if (!xr_result(result, "Failed to get System properties"))
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
  if (!xr_result(result, "Failed to get view configuration count"))
    return false;

  XrViewConfigurationType viewConfigurations[viewConfigurationCount];
  result = xrEnumerateViewConfigurations(
    self->instance, self->system_id, viewConfigurationCount,
    &viewConfigurationCount, viewConfigurations);
  if (!xr_result(result, "Failed to enumerate view configurations!"))
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
    if (!xr_result(result, "Failed to get view configuration info %d!", i))
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
  if (!xr_result(result, "Failed to get view configuration view count!"))
    return false;

  self->configuration_views =
    malloc(sizeof(XrViewConfigurationView) * self->view_count);

  result = xrEnumerateViewConfigurationViews(
    self->instance, self->system_id, self->view_config_type, self->view_count,
    &self->view_count, self->configuration_views);
  if (!xr_result(result, "Failed to enumerate view configuration views!"))
    return false;

  uint32_t secondaryViewConfigurationViewCount = 0;
  if (secondaryViewConfigProperties.type ==
      XR_TYPE_VIEW_CONFIGURATION_PROPERTIES) {

    result = xrEnumerateViewConfigurationViews(
      self->instance, self->system_id, optionalSecondaryViewConfigType, 0,
      &secondaryViewConfigurationViewCount, NULL);
    if (!xr_result(result, "Failed to get view configuration view count!"))
      return false;
  }

  if (secondaryViewConfigProperties.type ==
      XR_TYPE_VIEW_CONFIGURATION_PROPERTIES) {
    result = xrEnumerateViewConfigurationViews(
      self->instance, self->system_id, optionalSecondaryViewConfigType,
      secondaryViewConfigurationViewCount, &secondaryViewConfigurationViewCount,
      self->configuration_views);
    if (!xr_result(result, "Failed to enumerate view configuration views!"))
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
  if (!xr_result(result, "Failed to retrieve OpenXR Vulkan function pointer!"))
    return false;

  result =
    GetVulkanGraphicsRequirements(self->instance, self->system_id, &vk_reqs);
  if (!xr_result(result, "Failed to get Vulkan graphics requirements!"))
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
  if (!xr_result(result, "Failed to create session"))
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
  if (!xr_result (result, "Getting number of reference spaces failed!"))
    return false;

  XrReferenceSpaceType spaces[count];
  result = xrEnumerateReferenceSpaces (self->session, count, &count, spaces);
  if (!xr_result (result, "Enumerating reference spaces failed!"))
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
  if (!xr_result (result, "Failed to create local space."))
    return false;

  info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
  result = xrCreateReferenceSpace(self->session, &info, &self->view_space);
  if (!xr_result (result, "Failed to create view space."))
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
  if (!xr_result(result, "Failed to begin session!"))
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
  if (!xr_result(result, "Failed to get number of supported swapchain formats"))
    return false;

  int64_t swapchainFormats[swapchainFormatCount];
  result = xrEnumerateSwapchainFormats(self->session, swapchainFormatCount,
                                       &swapchainFormatCount, swapchainFormats);
  if (!xr_result(result, "Failed to enumerate swapchain formats"))
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
    if (!xr_result(result, "Failed to create swapchain %d!", i))
      return false;

    result = xrEnumerateSwapchainImages(self->swapchains[i], 0,
                                        &swapchainLength[i], NULL);
    if (!xr_result(result, "Failed to enumerate swapchains"))
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
    if (!xr_result(result, "Failed to enumerate swapchains"))
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

static bool
_create_actions(OpenXRContext *self)
{
  XrResult result;

  XrActionSetCreateInfo wm_actionset_info = {
    .type = XR_TYPE_ACTION_SET_CREATE_INFO,
    .next = NULL,
    .priority = 0
  };
  strcpy(wm_actionset_info.actionSetName, "wm");
  strcpy(wm_actionset_info.localizedActionSetName, "Window Management Action Set");

  result = xrCreateActionSet(self->instance, &wm_actionset_info, &self->wm_actionset);
  if (!xr_result(result, "failed to create wm actionset"))
    return false;

  xrStringToPath(self->instance, "/user/hand/left", &self->handPaths[0]);
  xrStringToPath(self->instance, "/user/hand/right", &self->handPaths[1]);

  XrActionCreateInfo actionInfo = {
    .type = XR_TYPE_ACTION_CREATE_INFO,
    .next = NULL,
    .actionType = XR_ACTION_TYPE_FLOAT_INPUT,
    .countSubactionPaths = NUM_CONTROLLERS,
    .subactionPaths = self->handPaths
  };
  // assuming every controller has some form of main "trigger" button
  strcpy(actionInfo.actionName, "triggergrab");
  strcpy(actionInfo.localizedActionName, "Grab Window with Trigger Button");

  result = xrCreateAction(self->wm_actionset, &actionInfo, &self->grabAction);
  if (!xr_result(result, "failed to create grab action"))
    return false;

  actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
  strcpy(actionInfo.actionName, "handpose");
  strcpy(actionInfo.localizedActionName, "Hand Pose");
  actionInfo.countSubactionPaths = NUM_CONTROLLERS;
  actionInfo.subactionPaths = self->handPaths;

  result = xrCreateAction(self->wm_actionset, &actionInfo, &self->poseAction);
  if (!xr_result(result, "failed to create pose action"))
    return false;

  XrPath grabInputPath[NUM_CONTROLLERS];
  xrStringToPath(self->instance, "/user/hand/left/input/select/click", &grabInputPath[0]);
  xrStringToPath(self->instance, "/user/hand/right/input/select/click", &grabInputPath[1]);

  XrPath poseInputPath[NUM_CONTROLLERS];
  xrStringToPath(self->instance, "/user/hand/left/input/grip/pose", &poseInputPath[0]);
  xrStringToPath(self->instance, "/user/hand/right/input/grip/pose", &poseInputPath[1]);

  XrPath khrSimpleInteractionProfilePath;
  result = xrStringToPath(self->instance, "/interaction_profiles/khr/simple_controller", &khrSimpleInteractionProfilePath);
  if (!xr_result(result, "failed to get interaction profile"))
    return false;

  const XrActionSuggestedBinding bindings[4] = {
    {
      .action = self->poseAction,
      .binding = poseInputPath[0]
    },
    {
      .action = self->poseAction,
      .binding = poseInputPath[1]
    },
    {
      .action = self->grabAction,
      .binding = grabInputPath[0]
    },
    {
      .action = self->grabAction,
      .binding = grabInputPath[1]
    }
  };

  const XrInteractionProfileSuggestedBinding suggestedBindings = {
    .type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING,
    .next = NULL,
    .interactionProfile = khrSimpleInteractionProfilePath,
    .countSuggestedBindings = 4,
    .suggestedBindings = bindings
  };

  xrSuggestInteractionProfileBindings(self->instance, &suggestedBindings);
  if (!xr_result(result, "failed to suggest bindings"))
    return false;

  XrActionSpaceCreateInfo actionSpaceInfo = {
    .type = XR_TYPE_ACTION_SPACE_CREATE_INFO,
    .next = NULL,
    .action = self->poseAction,
    .poseInActionSpace.orientation.w = 1.f,
    .subactionPath = self->handPaths[0]
  };

  result = xrCreateActionSpace(self->session, &actionSpaceInfo, &self->handSpaces[0]);
  if (!xr_result(result, "failed to create left hand pose space"))
    return false;

  actionSpaceInfo.subactionPath = self->handPaths[1];
  result = xrCreateActionSpace(self->session, &actionSpaceInfo, &self->handSpaces[1]);
  if (!xr_result(result, "failed to create right hand pose space"))
    return false;

  XrSessionActionSetsAttachInfo attachInfo = {
    .type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO,
    .next = NULL,
    .countActionSets = 1,
    .actionSets = &self->wm_actionset
  };
  result = xrAttachSessionActionSets(self->session, &attachInfo);
  if (!xr_result(result, "failed to attach action set"))
    return false;

  return true;
}

bool
openxr_context_begin_frame(OpenXRContext* self)
{
  XrResult result;

  XrEventDataBuffer runtimeEvent = {
    .type = XR_TYPE_EVENT_DATA_BUFFER,
    .next = NULL,
  };

  self->frame_state = (XrFrameState){
    .type = XR_TYPE_FRAME_STATE,
  };
  XrFrameWaitInfo frameWaitInfo = {
    .type = XR_TYPE_FRAME_WAIT_INFO,
  };
  result = xrWaitFrame(self->session, &frameWaitInfo, &self->frame_state);
  if (!xr_result(result, "xrWaitFrame() was not successful, exiting..."))
    return false;

  XrResult pollResult = xrPollEvent(self->instance, &runtimeEvent);
  if (pollResult == XR_SUCCESS) {
    switch (runtimeEvent.type) {
    case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
      XrEventDataSessionStateChanged* event =
        (XrEventDataSessionStateChanged*)&runtimeEvent;
      XrSessionState state = event->state;
      self->is_visible = event->state <= XR_SESSION_STATE_FOCUSED;
      g_debug("EVENT: session state changed to %d. Visible: %d\n", state,
              self->is_visible);
      if (event->state >= XR_SESSION_STATE_STOPPING) { // TODO
        self->is_runnting = false;
      }
      break;
    }
    default: break;
    }
  } else if (pollResult == XR_EVENT_UNAVAILABLE) {
    // this is the usual case
  } else {
    g_printerr("Failed to poll events!\n");
    return false;
  }
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
  };

  XrViewState viewState = {
    .type = XR_TYPE_VIEW_STATE,
  };
  uint32_t viewCountOutput;
  result = xrLocateViews(self->session, &viewLocateInfo, &viewState,
                         self->view_count, &viewCountOutput, self->views);
  if (!xr_result(result, "Could not locate views"))
    return false;

  // --- Begin frame
  XrFrameBeginInfo frameBeginInfo = {
    .type = XR_TYPE_FRAME_BEGIN_INFO,
  };

  result = xrBeginFrame(self->session, &frameBeginInfo);
  if (!xr_result(result, "failed to begin frame!"))
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
  if (!xr_result(result, "failed to acquire swapchain image!"))
    return false;

  XrSwapchainImageWaitInfo swapchainImageWaitInfo = {
    .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
    .timeout = INT64_MAX,
  };
  result = xrWaitSwapchainImage(self->swapchains[i], &swapchainImageWaitInfo);
  if (!xr_result(result, "failed to wait for swapchain image!"))
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
  if (!xr_result(result, "failed to release swapchain image!"))
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
  if (!xr_result(result, "failed to end frame!"))
    return false;

  free(self->views);

  return true;
}

void
openxr_context_cleanup(OpenXRContext* self)
{
  for (uint32_t i = 0; i < self->view_count; i++) {
    xrDestroySwapchain(self->swapchains[i]);
  }
  free(self->swapchains);
  xrDestroySpace(self->local_space);
  xrDestroySession(self->session);
  xrDestroyInstance(self->instance);
}

static void
openxr_context_finalize (GObject *gobject)
{
  OpenXRContext *self = OPENXR_CONTEXT (gobject);
  openxr_context_cleanup (self);
  G_OBJECT_CLASS (openxr_context_parent_class)->finalize (gobject);
}

bool
openxr_context_initialize(OpenXRContext* self,
                          VkInstance instance,
                          VkPhysicalDevice physical_device,
                          VkDevice device,
                          uint32_t queue_family_index,
                          uint32_t queue_index)
{
  if (!_check_vk_extension())
    return false;

  if (!_enumerate_api_layers())
    return false;

  if (!_create_instance(self))
    return false;

  if (!_create_system(self))
    return false;

  if (!_set_up_views(self))
    return false;

  if (!_check_graphics_api_support(self))
    return false;

  self->graphics_binding = (XrGraphicsBindingVulkanKHR){
    .type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR,
    .instance = instance,
    .physicalDevice = physical_device,
    .device = device,
    .queueFamilyIndex = queue_family_index,
    .queueIndex = queue_index,
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

  if (!_create_actions(self))
    return false;

  g_debug("Created wm actions\n");

  self->is_visible = true;
  self->is_runnting = true;

  self->projection_layer = (XrCompositionLayerProjection){
    .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
    .layerFlags = 0,
    .space = self->local_space,
    .viewCount = self->view_count,
    .views = self->projection_views,
  };

  self->is_initialized = true;

  return true;
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

static void
_get_model_matrix_from_pose (XrPosef *pose,
                             graphene_matrix_t *mat)
{
  graphene_quaternion_t q;
  graphene_quaternion_init (&q,
                            pose->orientation.x, pose->orientation.y,
                            pose->orientation.z, pose->orientation.w);

  graphene_matrix_init_identity (mat);
  graphene_matrix_rotate_quaternion (mat, &q);
  graphene_point3d_t translation = { .x = pose->position.x, pose->position.y, pose->position.z };
  graphene_matrix_translate(mat, &translation);
}

void
openxr_context_get_projection (OpenXRContext *self,
                               uint32_t i,
                               float near,
                               float far,
                               graphene_matrix_t *mat)
{
  if (!self->is_initialized)
    {
      g_warning ("OpenXR not initialized yet!\n");
      graphene_matrix_init_identity (mat);
      return;
    }
  _get_projection_matrix_from_fov (self->views[i].fov, near, far, mat);
}

void
openxr_context_get_view (OpenXRContext *self,
                         uint32_t i,
                         graphene_matrix_t *mat)
{
  if (!self->is_initialized)
    {
      g_warning ("OpenXR not initialized yet!\n");
      graphene_matrix_init_identity (mat);
      return;
    }
  _get_view_matrix_from_pose (&self->views[i].pose, mat);
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

bool
_space_location_valid (XrSpaceLocation *sl)
{
  return (sl->locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)    != 0 &&
         (sl->locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0;
}

gboolean
openxr_context_get_head_pose (OpenXRContext     *self,
                              graphene_matrix_t *pose)
{
  XrSpaceLocation space_location = {
    .type = XR_TYPE_SPACE_LOCATION,
    .next = NULL
  };

  XrResult result = xrLocateSpace (self->view_space, self->local_space,
                                   self->frame_state.predictedDisplayTime,
                                  &space_location);
  xr_result (result, "Failed to locate head space.");

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

void
openxr_context_poll_controllers (OpenXRContext *self)
{
  XrResult result;

  /* TODO: only sync once per frame */
  const XrActiveActionSet activeActionSet = {
    .actionSet = self->wm_actionset,
    .subactionPath = XR_NULL_PATH
  };

  XrActionsSyncInfo syncInfo = {
    .type = XR_TYPE_ACTIONS_SYNC_INFO,
    .countActiveActionSets = 1,
    .activeActionSets = &activeActionSet
  };
  result = xrSyncActions(self->session, &syncInfo);
  xr_result(result, "failed to sync actions!");

  for (int i = 0; i < NUM_CONTROLLERS; i++)
    {
      XrActionStateGetInfo getInfo = {
        .type = XR_TYPE_ACTION_STATE_GET_INFO,
        .next = NULL,
        .action = self->grabAction,
        .subactionPath = self->handPaths[i]
      };

      getInfo.action = self->poseAction;
      XrActionStatePose poseState = {
        .type = XR_TYPE_ACTION_STATE_POSE,
        .next = NULL
      };
      result = xrGetActionStatePose(self->session, &getInfo, &poseState);
      xr_result(result, "failed to get pose value!");

      XrSpaceLocation spaceLocation;
      bool spaceLocationValid;
      spaceLocation.type = XR_TYPE_SPACE_LOCATION;
      spaceLocation.next = NULL;

      result = xrLocateSpace(self->handSpaces[i], self->local_space,
                             self->frame_state.predictedDisplayTime,
                             &spaceLocation);
      xr_result(result, "failed to locate hand space %d!", i);
      spaceLocationValid = _space_location_valid (&spaceLocation);

      // TODO: merge with openvr action
      if (spaceLocationValid && poseState.isActive)
        {
          GxrPoseEvent *event = g_malloc (sizeof (GxrPoseEvent));
          event->active = poseState.isActive;
          event->controller_handle = i;
          _get_model_matrix_from_pose(&spaceLocation.pose, &event->pose);

          graphene_vec3_init (&event->velocity, 0, 0, 0);
          graphene_vec3_init (&event->angular_velocity, 0, 0, 0);

          event->valid = spaceLocationValid;
          event->device_connected = true;

          g_debug ("Controller %i pose valid: %d, pos %f %f %f",
                   i, event->valid, spaceLocation.pose.position.x,
                   spaceLocation.pose.position.y,
                   spaceLocation.pose.position.z);

          g_signal_emit (self, action_signals[POSE_EVENT], 0, event);
        }

      getInfo.action = self->grabAction,
      getInfo.subactionPath = self->handPaths[i];
      XrActionStateFloat grabValue = {
        .type = XR_TYPE_ACTION_STATE_FLOAT,
        .next = NULL
      };
      result = xrGetActionStateFloat(self->session, &getInfo, &grabValue);
      xr_result(result, "failed to get grab value!");
      if (grabValue.isActive)
        {
          GxrDigitalEvent *event = g_malloc (sizeof (GxrDigitalEvent));
          event->controller_handle = i;
          event->active = grabValue.isActive;
          event->state = grabValue.currentState > 0.5; /* TODO */
          event->changed = grabValue.changedSinceLastSync;
          event->time = 0;

          g_debug ("Grabvalue (active %d): %f, changed: %d\n",
                   grabValue.isActive, grabValue.currentState, event->changed);

          g_signal_emit (self, action_signals[DIGITAL_EVENT], 0, event);
        }
    }
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
_get_frustum_angles (GxrEye eye,
                     float *left, float *right,
                     float *top, float *bottom)
{
  (void) eye;
  *left = 1, *right = 1, *top = 1, *bottom = 1;
  g_warning ("_get_frustum_angles not implemented in OpenXR.\n");
}

static gboolean
_get_head_pose (graphene_matrix_t *pose)
{
  OpenXRContext *self = OPENXR_CONTEXT (gxr_context_get_instance ());
  return openxr_context_get_head_pose (self, pose);
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

  action_signals[DIGITAL_EVENT] =
  g_signal_new ("grab-event", /* TODO: binding, digital-event */
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL, G_TYPE_NONE,
                1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  action_signals[ANALOG_EVENT] =
  g_signal_new ("analog-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL, G_TYPE_NONE,
                1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  action_signals[POSE_EVENT] =
  g_signal_new ("pose-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, NULL, G_TYPE_NONE,
                1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
}
