/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-context-private.h"

#define XR_USE_PLATFORM_XLIB 1
#define XR_USE_GRAPHICS_API_VULKAN 1
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>

#include "gxr-controller.h"
#include "gxr-version.h"

// TODO: Do not hardcode this
#define NUM_CONTROLLERS 2

enum GxrSwapchainType
{
  GxrSwapchainTypeColor = 0,
  GxrSwapchainTypeDepth = 1,
  GxrSwapchainTypeLast,
};

struct GxrSwapchain
{
  /* One array layer per view */
  uint32_t                   array_size;
  XrSwapchain                handle;
  XrSwapchainImageVulkanKHR *images;
  /* last acquired swapchain image index per swapchain */
  uint32_t buffer_index;
  /* for each view */
  uint32_t length;
  VkFormat format;

  enum GxrSwapchainType swapchain_type;
};

struct _GxrContext
{
  GObject        parent;
  GulkanContext *gc;

  struct
  {
    gboolean vulkan_enable2;
    gboolean overlay;
    gboolean depth;
  } extensions;
  XrEnvironmentBlendMode blend_mode;

  GxrDeviceManager *device_manager;

  XrInstance              instance;
  XrSession               session;
  XrReferenceSpaceType    play_space_type;
  XrSpace                 play_space;
  XrSpace                 view_space;
  XrSystemId              system_id;
  XrViewConfigurationType view_config_type;

  // We use one swapchain per type, use GxrSwapchainType as index
  struct GxrSwapchain swapchain[GxrSwapchainTypeLast];

  /* 1 framebuffer for each swapchain image, for each swapchain (1 per view) */
  GulkanFrameBuffer   **framebuffers;
  VkExtent2D            framebuffer_extent;
  VkSampleCountFlagBits framebuffer_sample_count;

  XrCompositionLayerDepthInfoKHR   *depth_infos;
  XrCompositionLayerProjectionView *projection_views;
  XrViewConfigurationView          *configuration_views;

  XrGraphicsBindingVulkanKHR graphics_binding;

  XrSessionCreateInfoOverlayEXTX overlay_info;

  uint32_t view_count;

  XrSessionState session_state;
  gboolean       should_render;
  gboolean       have_valid_pose;
  // to avoid beginning an already running session
  gboolean session_running;
  // run begin/end frame cycle only when we are in certain states
  gboolean should_submit_frames;

  XrCompositionLayerProjection projection_layer;

  volatile XrTime     predicted_display_time;
  volatile XrDuration predicted_display_period;

  XrView *views;

  XrVersion desired_vk_version;

  GMutex wait_frame_mutex;
};

G_DEFINE_TYPE (GxrContext, gxr_context, G_TYPE_OBJECT)

enum
{
  STATE_CHANGE_EVENT,
  OVERLAY_EVENT,
  LAST_SIGNAL
};

static guint context_signals[LAST_SIGNAL] = {0};

static void
gxr_context_finalize (GObject *gobject);

static void
gxr_context_class_init (GxrContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gxr_context_finalize;

  context_signals[STATE_CHANGE_EVENT]
    = g_signal_new ("state-change-event", G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1,
                    G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE);

  context_signals[OVERLAY_EVENT]
    = g_signal_new ("overlay-event", G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1,
                    G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static const char *viewport_config_name = "/viewport_configuration/vr";

static const char *
_check_xr_result_to_string (XrResult result)
{
  switch (result)
    {

#define MAKE_CASE(VAL, _)                                                      \
  case VAL:                                                                    \
    return #VAL;

      XR_LIST_ENUM_XrResult (MAKE_CASE) default : return "UNKNOWN";
    }
}

#define BUF_LEN 1024
static gboolean
_check_xr_result (XrResult result, const char *format, ...)
{
  if (XR_SUCCEEDED (result))
    return TRUE;

  const char *result_str = _check_xr_result_to_string (result);

  char msg[BUF_LEN] = {0};
  g_snprintf (msg, BUF_LEN, "[%s] ", result_str);

  gulong result_written_len = (gulong) strlen (msg);

  va_list args;
  va_start (args, format);
  g_vsnprintf (msg + result_written_len, BUF_LEN - result_written_len, format,
               args);
  va_end (args);

  g_warning ("%s", msg);
  return FALSE;
}

static gboolean
_is_extension_supported (char                  *name,
                         XrExtensionProperties *props,
                         uint32_t               count)
{
  for (uint32_t i = 0; i < count; i++)
    if (!strcmp (name, props[i].extensionName))
      return TRUE;
  return FALSE;
}

static gboolean
_check_extensions (GxrContext *self)
{
  XrResult result;
  uint32_t instanceExtensionCount = 0;
  result = xrEnumerateInstanceExtensionProperties (NULL, 0,
                                                   &instanceExtensionCount,
                                                   NULL);

  if (!_check_xr_result (result, "Failed to enumerate number of instance "
                                 "extension properties"))
    return FALSE;

  XrExtensionProperties *instanceExtensionProperties
    = g_malloc (sizeof (XrExtensionProperties) * instanceExtensionCount);
  for (uint16_t i = 0; i < instanceExtensionCount; i++)
    instanceExtensionProperties[i] = (XrExtensionProperties){
      .type = XR_TYPE_EXTENSION_PROPERTIES,
    };

  result = xrEnumerateInstanceExtensionProperties (NULL, instanceExtensionCount,
                                                   &instanceExtensionCount,
                                                   instanceExtensionProperties);
  if (!_check_xr_result (result, "Failed to enumerate extension properties"))
    return FALSE;

  self->extensions.vulkan_enable2
    = _is_extension_supported (XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME,
                               instanceExtensionProperties,
                               instanceExtensionCount);

  self->extensions.depth
    = _is_extension_supported (XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME,
                               instanceExtensionProperties,
                               instanceExtensionCount);
  g_debug ("%s extension supported: %d",
           XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME,
           self->extensions.depth);

  self->extensions.overlay
    = _is_extension_supported (XR_EXTX_OVERLAY_EXTENSION_NAME,
                               instanceExtensionProperties,
                               instanceExtensionCount);
  g_debug ("%s extension supported: %d", XR_EXTX_OVERLAY_EXTENSION_NAME,
           self->extensions.overlay);

  g_free (instanceExtensionProperties);

  if (!self->extensions.vulkan_enable2)
    {
      g_printerr ("Runtime does not support instance extension %s\n",
                  XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);
      return FALSE;
    }

  return TRUE;
}

static gboolean
_check_blend_mode (GxrContext *self)
{
  XrResult result;
  uint32_t blend_modes_count = 0;
  result = xrEnumerateEnvironmentBlendModes (self->instance, self->system_id,
                                             self->view_config_type, 0,
                                             &blend_modes_count, NULL);
  if (!_check_xr_result (result, "Failed to get blend modes count"))
    return FALSE;

  XrEnvironmentBlendMode *blend_modes
    = g_malloc (sizeof (XrEnvironmentBlendMode) * blend_modes_count);

  result = xrEnumerateEnvironmentBlendModes (self->instance, self->system_id,
                                             self->view_config_type,
                                             blend_modes_count,
                                             &blend_modes_count, blend_modes);
  if (!_check_xr_result (result, "Failed to get blend modes"))
    {
      g_free (blend_modes);
      return FALSE;
    }

  g_debug ("Supported environment blend modes");
  for (uint32_t i = 0; i < blend_modes_count; i++)
    {
      g_debug ("\t%d", blend_modes[i]);
    }

  g_info ("Using environment blend mode  %d", blend_modes[0]);
  self->blend_mode = blend_modes[0];

  g_free (blend_modes);

  return TRUE;
}

static gboolean
_create_instance (GxrContext *self, char *app_name, uint32_t app_version)
{

  const char *enabled_extensions[10] = {
    XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME,
  };
  // vulkan_enable2 is required
  uint32_t enabled_extension_count = 1;

  if (self->extensions.overlay)
    {
      enabled_extensions[enabled_extension_count++]
        = XR_EXTX_OVERLAY_EXTENSION_NAME;
    }

  if (self->extensions.depth)
    {
      enabled_extensions[enabled_extension_count++]
        = XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME;
    }

  XrInstanceCreateInfo instanceCreateInfo = {
    .type = XR_TYPE_INSTANCE_CREATE_INFO,
    .createFlags = 0,
    .enabledExtensionCount = enabled_extension_count,
    .enabledExtensionNames = enabled_extensions,
    .enabledApiLayerCount = 0,
    .applicationInfo = {
      .applicationVersion = app_version,
      .engineName = "gxr",
      .engineVersion = GXR_VERSION_HEX,
      .apiVersion = XR_CURRENT_API_VERSION,
    },
  };

  strncpy (instanceCreateInfo.applicationInfo.applicationName, app_name,
           XR_MAX_APPLICATION_NAME_SIZE);

  XrResult result;
  result = xrCreateInstance (&instanceCreateInfo, &self->instance);
  if (!_check_xr_result (result, "Failed to create XR instance."))
    return FALSE;

  return TRUE;
}

static gboolean
_create_system (GxrContext *self)
{
  XrPath   vrConfigName;
  XrResult result;
  result = xrStringToPath (self->instance, viewport_config_name, &vrConfigName);
  _check_xr_result (result, "failed to get viewport configuration name");

  g_debug ("Got vrconfig %lu", vrConfigName);

  XrSystemGetInfo systemGetInfo = {
    .type = XR_TYPE_SYSTEM_GET_INFO,
    .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
  };

  result = xrGetSystem (self->instance, &systemGetInfo, &self->system_id);
  if (!_check_xr_result (result,
                         "Failed to get system for %s viewport configuration.",
                         viewport_config_name))
    return FALSE;

  XrSystemProperties systemProperties = {
    .type = XR_TYPE_SYSTEM_PROPERTIES,
    .graphicsProperties = {0},
    .trackingProperties = {0},
  };

  result = xrGetSystemProperties (self->instance, self->system_id,
                                  &systemProperties);
  if (!_check_xr_result (result, "Failed to get System properties"))
    return FALSE;

  return TRUE;
}

static gboolean
_set_up_views (GxrContext *self)
{
  uint32_t viewConfigurationCount;
  XrResult result;
  result = xrEnumerateViewConfigurations (self->instance, self->system_id, 0,
                                          &viewConfigurationCount, NULL);
  if (!_check_xr_result (result, "Failed to get view configuration count"))
    return FALSE;

  XrViewConfigurationType *viewConfigurations
    = g_malloc (sizeof (XrViewConfigurationType) * viewConfigurationCount);
  result = xrEnumerateViewConfigurations (self->instance, self->system_id,
                                          viewConfigurationCount,
                                          &viewConfigurationCount,
                                          viewConfigurations);
  if (!_check_xr_result (result, "Failed to enumerate view configurations!"))
    return FALSE;

  self->view_config_type = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

  /* if struct (more specifically .type) is still 0 after searching, then
   we have not found the config. This way we don't need to set a bool
   found to TRUE. */
  XrViewConfigurationProperties requiredViewConfigProperties = {0};

  for (uint32_t i = 0; i < viewConfigurationCount; ++i)
    {
      XrViewConfigurationProperties properties = {
        .type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES,
      };

      result = xrGetViewConfigurationProperties (self->instance,
                                                 self->system_id,
                                                 viewConfigurations[i],
                                                 &properties);
      if (!_check_xr_result (result,
                             "Failed to get view configuration info %d!", i))
        return FALSE;

      if (viewConfigurations[i] == self->view_config_type
          && properties.viewConfigurationType == self->view_config_type)
        requiredViewConfigProperties = properties;
    }

  g_free (viewConfigurations);

  if (requiredViewConfigProperties.type
      != XR_TYPE_VIEW_CONFIGURATION_PROPERTIES)
    {
      g_print ("Couldn't get required VR View Configuration %s from Runtime!\n",
               viewport_config_name);
      return FALSE;
    }

  result = xrEnumerateViewConfigurationViews (self->instance, self->system_id,
                                              self->view_config_type, 0,
                                              &self->view_count, NULL);

  self->views = g_malloc (sizeof (XrView) * self->view_count);
  for (uint32_t i = 0; i < self->view_count; i++)
    {
      self->views[i].type = XR_TYPE_VIEW;
      self->views[i].next = NULL;
    }

  if (!_check_xr_result (result,
                         "Failed to get view configuration view count!"))
    return FALSE;

  self->configuration_views = g_malloc (sizeof (XrViewConfigurationView)
                                        * self->view_count);

  for (uint32_t i = 0; i < self->view_count; i++)
    {
      self->configuration_views[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
      self->configuration_views[i].next = NULL;
    }

  result = xrEnumerateViewConfigurationViews (self->instance, self->system_id,
                                              self->view_config_type,
                                              self->view_count,
                                              &self->view_count,
                                              self->configuration_views);
  if (!_check_xr_result (result,
                         "Failed to enumerate view configuration views!"))
    return FALSE;

  return TRUE;
}

static gboolean
_check_graphics_api_support (GxrContext *self)
{

  // same aliased struct and type for vulkan_enable and vulkan_enable2
  XrGraphicsRequirementsVulkanKHR vk_reqs = {
    .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR,
  };

  PFN_xrGetVulkanGraphicsRequirementsKHR GetVulkanGraphicsRequirements2 = NULL;
  XrResult                               res;
  res = xrGetInstanceProcAddr (self->instance,
                               "xrGetVulkanGraphicsRequirements2KHR",
                               (PFN_xrVoidFunction
                                  *) (&GetVulkanGraphicsRequirements2));
  if (!_check_xr_result (res, "Failed to retrieve "
                              "xrGetVulkanGraphicsRequirements2KHR pointer!"))
    return FALSE;

  res = GetVulkanGraphicsRequirements2 (self->instance, self->system_id,
                                        &vk_reqs);
  if (!_check_xr_result (res, "Failed to get Vulkan graphics requirements!"))
    return FALSE;

  if (self->desired_vk_version > vk_reqs.maxApiVersionSupported
      || self->desired_vk_version < vk_reqs.minApiVersionSupported)
    {
      g_warning ("Runtime does not support requested Vulkan version %lu",
                 self->desired_vk_version);
      g_warning ("We will use maxApiVersionSupported %lu",
                 vk_reqs.maxApiVersionSupported);
      self->desired_vk_version = vk_reqs.maxApiVersionSupported;
    }
  return TRUE;
}

static gboolean
_init_runtime (GxrContext *self, char *app_name, uint32_t app_version)
{
  if (!_check_extensions (self))
    return FALSE;

  if (!_create_instance (self, app_name, app_version))
    return FALSE;

  if (!_create_system (self))
    return FALSE;

  if (!_set_up_views (self))
    return FALSE;

  if (!_check_graphics_api_support (self))
    return FALSE;

  if (!_check_blend_mode (self))
    return FALSE;

  return TRUE;
}

static gboolean
_create_session (GxrContext *self)
{
  XrSessionCreateInfo session_create_info = {
    .type = XR_TYPE_SESSION_CREATE_INFO,
    .next = &self->graphics_binding,
    .systemId = self->system_id,
  };

  XrResult result = xrCreateSession (self->instance, &session_create_info,
                                     &self->session);
  if (!_check_xr_result (result, "Failed to create session"))
    return FALSE;
  return TRUE;
}

static gboolean
_is_space_supported (XrReferenceSpaceType *spaces,
                     uint32_t              count,
                     XrReferenceSpaceType  type)
{
  for (uint32_t i = 0; i < count; i++)
    if (spaces[i] == type)
      return TRUE;
  return FALSE;
}

static gboolean
_check_supported_spaces (GxrContext *self)
{
  uint32_t count;
  XrResult result = xrEnumerateReferenceSpaces (self->session, 0, &count, NULL);
  if (!_check_xr_result (result, "Getting number of reference spaces failed!"))
    return FALSE;

  XrReferenceSpaceType *spaces = g_malloc (sizeof (XrReferenceSpaceType)
                                           * count);
  result = xrEnumerateReferenceSpaces (self->session, count, &count, spaces);
  if (!_check_xr_result (result, "Enumerating reference spaces failed!"))
    return FALSE;

  XrReferenceSpaceType space_type = XR_REFERENCE_SPACE_TYPE_STAGE;
  const gchar         *gxr_space = g_getenv ("GXR_SPACE");
  if (gxr_space)
    {
      if (g_strcmp0 (gxr_space, "LOCAL") == 0)
        space_type = XR_REFERENCE_SPACE_TYPE_LOCAL;
    }

  if (_is_space_supported (spaces, count, space_type))
    {
      self->play_space_type = space_type;
      g_debug ("Requested play space supported.");
    }
  else
    {
      self->play_space_type = spaces[0];
      g_debug ("Requested play space not supported, fall back to %d!",
               spaces[0]);
    }

  if (!_is_space_supported (spaces, count, XR_REFERENCE_SPACE_TYPE_VIEW))
    {
      g_print ("XR_REFERENCE_SPACE_TYPE_VIEW unsupported.\n");
      return FALSE;
    }

  g_free (spaces);

  XrPosef space_pose = {
    .orientation = {.x = 0, .y = 0, .z = 0, .w = 1.0},
    .position = {.x = 0, .y = 0, .z = 0},
  };

  // TODO: monado doesn't handle this well
#if 0
  if (self->play_space_type == XR_REFERENCE_SPACE_TYPE_LOCAL)
    {
      space_pose.position.y = -1.6f;
      g_debug ("Using local space with %f y offset", space_pose.position.y);
    }
#endif

  XrReferenceSpaceCreateInfo info = {
    .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
    .referenceSpaceType = self->play_space_type,
    .poseInReferenceSpace = space_pose,
  };
  result = xrCreateReferenceSpace (self->session, &info, &self->play_space);
  if (!_check_xr_result (result, "Failed to create local space."))
    return FALSE;

  info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
  result = xrCreateReferenceSpace (self->session, &info, &self->view_space);
  if (!_check_xr_result (result, "Failed to create view space."))
    return FALSE;

  return TRUE;
}

static gboolean
_begin_session (GxrContext *self)
{
  XrSessionBeginInfo sessionBeginInfo = {
    .type = XR_TYPE_SESSION_BEGIN_INFO,
    .primaryViewConfigurationType = self->view_config_type,
  };
  XrResult result = xrBeginSession (self->session, &sessionBeginInfo);
  if (!_check_xr_result (result, "Failed to begin session!"))
    return FALSE;

  self->session_running = TRUE;
  return TRUE;
}

static gboolean
_end_session (GxrContext *self)
{
  XrResult result = xrEndSession (self->session);
  if (!_check_xr_result (result, "Failed to end session!"))
    return FALSE;

  self->session_running = FALSE;
  return TRUE;
}

static gboolean
_destroy_session (GxrContext *self)
{
  XrResult result = xrDestroySession (self->session);
  if (!_check_xr_result (result, "Failed to destroy session!"))
    return FALSE;

  self->session_running = FALSE;
  self->session = NULL;
  return TRUE;
}

static gboolean
_create_swapchain (GxrContext           *self,
                   struct GxrSwapchain  *swapchain,
                   enum GxrSwapchainType swapchain_type)
{
  XrResult result;
  uint32_t swapchainFormatCount;
  result = xrEnumerateSwapchainFormats (self->session, 0, &swapchainFormatCount,
                                        NULL);
  if (!_check_xr_result (result,
                         "Failed to get number of supported swapchain formats"))
    return FALSE;

  int64_t *swapchainFormats = g_malloc (sizeof (int64_t)
                                        * swapchainFormatCount);
  result = xrEnumerateSwapchainFormats (self->session, swapchainFormatCount,
                                        &swapchainFormatCount,
                                        swapchainFormats);
  if (!_check_xr_result (result, "Failed to enumerate swapchain formats"))
    {
      g_free (swapchainFormats);
      return FALSE;
    }

  g_debug ("Supported swapchain formats:");
  for (uint32_t i = 0; i < swapchainFormatCount; i++)
    g_debug ("%s", vk_format_string ((VkFormat) swapchainFormats[i]));

  VkFormat preferred_format[2];
  uint32_t preferred_format_count = 0;

  XrSwapchainUsageFlags usage_flags = 0;

  switch (swapchain_type)
    {
      case GxrSwapchainTypeColor:
        preferred_format[0] = VK_FORMAT_R8G8B8A8_SRGB;
        preferred_format_count = 1;
        usage_flags |= XR_SWAPCHAIN_USAGE_SAMPLED_BIT
                       | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        break;
      case GxrSwapchainTypeDepth:
        preferred_format[0] = VK_FORMAT_D32_SFLOAT;
        preferred_format[1] = VK_FORMAT_D16_UNORM;
        preferred_format_count = 2;
        usage_flags |= XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        break;
      case GxrSwapchainTypeLast:
        break;
    }

  swapchain->format = VK_FORMAT_UNDEFINED;

  for (uint32_t i = 0; i < preferred_format_count; i++)
    {
      for (uint32_t j = 0; j < swapchainFormatCount; j++)
        {
          if (preferred_format[i] == swapchainFormats[j])
            {
              swapchain->format = (VkFormat) swapchainFormats[j];
              break;
            }
        }
      if (swapchain->format != VK_FORMAT_UNDEFINED)
        break;
    }

  if (swapchain->format == VK_FORMAT_UNDEFINED
      && swapchain_type == GxrSwapchainTypeColor)
    {
      for (uint32_t i = 0; i < preferred_format_count; i++)
        {
          g_warning ("Requested %s, but runtime doesn't support it.",
                     vk_format_string ((VkFormat) preferred_format[i]));
        }
      g_warning ("Using %s instead.",
                 vk_format_string ((VkFormat) swapchainFormats[0]));
      swapchain->format = (VkFormat) swapchainFormats[0];
    }

  if (swapchain->format == VK_FORMAT_UNDEFINED
      && swapchain_type == GxrSwapchainTypeDepth)
    {
      for (uint32_t i = 0; i < preferred_format_count; i++)
        {
          g_warning ("Requested %s, but runtime doesn't support it.",
                     vk_format_string ((VkFormat) preferred_format[i]));
        }
      return FALSE;
    }

  /* make sure we don't clean up uninitialized pointer on failure */
  swapchain->images = NULL;

  XrSwapchainCreateInfo swapchainCreateInfo = {
    .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
    .usageFlags = usage_flags,
    .createFlags = 0,
    .format = swapchain->format,
    .sampleCount = 1,
    .width = self->configuration_views[0].recommendedImageRectWidth,
    .height = self->configuration_views[0].recommendedImageRectHeight,
    .faceCount = 1,
    .arraySize = 2,
    .mipCount = 1,
  };

  g_debug ("Swapchain %d dimensions: %dx%d", 0,
           self->configuration_views[0].recommendedImageRectWidth,
           self->configuration_views[0].recommendedImageRectHeight);

  result = xrCreateSwapchain (self->session, &swapchainCreateInfo,
                              &swapchain->handle);
  if (!_check_xr_result (result, "Failed to create swapchain %d!", 0))
    {
      g_free (swapchainFormats);
      return FALSE;
    }

  result = xrEnumerateSwapchainImages (swapchain->handle, 0, &swapchain->length,
                                       NULL);
  if (!_check_xr_result (result, "Failed to enumerate swapchains"))
    {
      g_free (swapchainFormats);
      return FALSE;
    }

  swapchain->images = g_malloc (sizeof (XrSwapchainImageVulkanKHR)
                                * swapchain->length);

  for (uint32_t j = 0; j < swapchain->length; j++)
    {
      // ...IMAGE_VULKAN2_KHR = ...IMAGE_VULKAN_KHR
      swapchain->images[j].type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
      swapchain->images[j].next = NULL;
    }

  result = xrEnumerateSwapchainImages (swapchain->handle, swapchain->length,
                                       &swapchain->length,
                                       (XrSwapchainImageBaseHeader *)
                                         swapchain->images);
  if (!_check_xr_result (result, "Failed to enumerate swapchain images"))
    {
      g_free (swapchainFormats);
      return FALSE;
    }

  g_free (swapchainFormats);

  swapchain->swapchain_type = swapchain_type;

  return TRUE;
}

static gboolean
_create_swapchains (GxrContext *self)
{
  if (!_create_swapchain (self, &self->swapchain[GxrSwapchainTypeColor],
                          GxrSwapchainTypeColor))
    {
      g_printerr ("Failed to create color swapchain");
      return FALSE;
    }

  if (!_create_swapchain (self, &self->swapchain[GxrSwapchainTypeDepth],
                          GxrSwapchainTypeDepth))
    {
      g_printerr ("Failed to create depth swapchain");
      return FALSE;
    }

  return TRUE;
}

static void
_create_projection_views (GxrContext *self)
{
  self->projection_views = g_malloc (sizeof (XrCompositionLayerProjectionView)
                                     * self->view_count);

  for (uint32_t i = 0; i < self->view_count; i++)
    self->projection_views[i] = (XrCompositionLayerProjectionView) {
      .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
      .subImage = {
        .swapchain = self->swapchain[GxrSwapchainTypeColor].handle,
        .imageRect = {
          .extent = {
              .width = (int32_t) self->configuration_views[i].recommendedImageRectWidth,
              .height = (int32_t) self->configuration_views[i].recommendedImageRectHeight,
          },
        },
        .imageArrayIndex = i,
      },
    };

  if (self->extensions.depth)
    {
      self->depth_infos = g_malloc (sizeof (XrCompositionLayerDepthInfoKHR)
                                    * self->view_count);

      for (uint32_t i = 0; i < self->view_count; i++)
        {
          self->depth_infos[i] = (XrCompositionLayerDepthInfoKHR)
          {
            .type = XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR,
          .subImage = {
            .swapchain = self->swapchain[GxrSwapchainTypeDepth].handle,
            .imageRect = {
              .extent = {
                .width = (int32_t) self->configuration_views[i].recommendedImageRectWidth,
                .height = (int32_t) self->configuration_views[i].recommendedImageRectHeight,
              },
            },
            .imageArrayIndex = i,
          },
          };

          self->projection_views[i].next = &self->depth_infos[i];
        }
    }
}

static gboolean
_init_session (GxrContext *self)
{
  GulkanContext *gc = gxr_context_get_gulkan (self);
  GulkanDevice  *gd = gulkan_context_get_device (gc);
  GulkanQueue   *queue = gulkan_device_get_graphics_queue (gd);

  uint32_t family_index = gulkan_queue_get_family_index (queue);

  self->graphics_binding = (XrGraphicsBindingVulkanKHR){
    .type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR,
    .instance = gulkan_context_get_instance_handle (gc),
    .physicalDevice = gulkan_device_get_physical_handle (gd),
    .device = gulkan_device_get_handle (gd),
    .queueFamilyIndex = family_index,
    .queueIndex = 0,
  };

  // TODO: session layer placement should be configurable
  self->overlay_info = (XrSessionCreateInfoOverlayEXTX){
    .type = XR_TYPE_SESSION_CREATE_INFO_OVERLAY_EXTX,
    .next = NULL,
    .sessionLayersPlacement = 1,
  };

  if (self->extensions.overlay)
    {
      self->graphics_binding.next = &self->overlay_info;
    }

  if (!_create_session (self))
    return FALSE;

  if (!_check_supported_spaces (self))
    return FALSE;

  if (!_begin_session (self))
    return FALSE;

  if (!_create_swapchains (self))
    return FALSE;

  g_print ("Created swapchains.\n");

  _create_projection_views (self);

  self->swapchain[GxrSwapchainTypeColor].buffer_index = 0;
  self->swapchain[GxrSwapchainTypeDepth].buffer_index = 0;

  self->session_state = XR_SESSION_STATE_UNKNOWN;
  self->should_render = FALSE;
  self->have_valid_pose = FALSE;

  self->projection_layer = (XrCompositionLayerProjection){
    .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
    .layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT,
    .space = self->play_space,
    .viewCount = self->view_count,
    .views = self->projection_views,
  };

  return TRUE;
}

static void
_remove_unsupported_exts (GSList               **target,
                          uint32_t               supported_count,
                          VkExtensionProperties *supported_props)
{
  for (GSList *l = *target; l; l = l->next)
    {
      gboolean is_supported = FALSE;
      for (uint32_t i = 0; i < supported_count; i++)
        {
          gchar *ext = l->data;
          if (strcmp (supported_props[i].extensionName, ext) == 0)
            {
              is_supported = TRUE;
              break;
            }
        }
      if (!is_supported)
        {
          g_print ("Disabling unsupported ext %s\n", (gchar *) l->data);
          *target = g_slist_delete_link (*target, l);
        }
    }
}

static void
_remove_unsupported_instance_exts (GSList **target)
{
  uint32_t supported_count;
  vkEnumerateInstanceExtensionProperties (NULL, &supported_count, NULL);

  VkExtensionProperties *supported_props
    = g_malloc (sizeof (VkExtensionProperties) * supported_count);
  vkEnumerateInstanceExtensionProperties (NULL, &supported_count,
                                          supported_props);

  _remove_unsupported_exts (target, supported_count, supported_props);
  g_free (supported_props);
}

static void
_remove_unsupported_device_exts (VkPhysicalDevice vk_physical_device,
                                 GSList         **target)
{
  uint32_t supported_count;
  vkEnumerateDeviceExtensionProperties (vk_physical_device, NULL,
                                        &supported_count, NULL);

  VkExtensionProperties *supported_props
    = g_malloc (sizeof (VkExtensionProperties) * supported_count);
  vkEnumerateDeviceExtensionProperties (vk_physical_device, NULL,
                                        &supported_count, supported_props);

  _remove_unsupported_exts (target, supported_count, supported_props);
  g_free (supported_props);
}

static gpointer
_copy_str (gconstpointer src, gpointer data)
{
  (void) data;
  return g_strdup (src);
}

static uint32_t
_xr_to_vk_version (XrVersion version)
{
#if VK_HEADER_VERSION >= 175
  return VK_MAKE_API_VERSION (0, XR_VERSION_MAJOR (version),
                              XR_VERSION_MINOR (version),
                              XR_VERSION_PATCH (version));
#else
  return VK_MAKE_VERSION (XR_VERSION_MAJOR (version),
                          XR_VERSION_MINOR (version),
                          XR_VERSION_PATCH (version));
#endif
}

static gboolean
_create_vk_instance2 (GxrContext *self,
                      GSList     *instance_ext_list,
                      VkInstance *instance)
{
  VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "gxr",
    .pEngineName = "gxr",
    .apiVersion = _xr_to_vk_version (self->desired_vk_version),
  };

  GSList *instance_ext_list_reduced = g_slist_copy_deep (instance_ext_list,
                                                         _copy_str, NULL);
  _remove_unsupported_instance_exts (&instance_ext_list_reduced);

  uint32_t     num_extensions = g_slist_length (instance_ext_list_reduced);
  const char **extension_names = g_malloc (sizeof (char *) * num_extensions);

  uint32_t i = 0;
  for (GSList *l = instance_ext_list_reduced; l; l = l->next)
    extension_names[i++] = l->data;

  // runtime will add extensions it requires
  VkInstanceCreateInfo instance_info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &app_info,
    .enabledExtensionCount = num_extensions,
    .ppEnabledExtensionNames = extension_names,
  };

  XrVulkanInstanceCreateInfoKHR xr_vk_instance_info = {
    .type = XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR,
    .next = NULL,
    .createFlags = 0,
    .pfnGetInstanceProcAddr = vkGetInstanceProcAddr,
    .systemId = self->system_id,
    .vulkanCreateInfo = &instance_info,
    .vulkanAllocator = NULL,
  };

  PFN_xrCreateVulkanInstanceKHR CreateVulkanInstanceKHR = NULL;
  XrResult                      res;
  res = xrGetInstanceProcAddr (self->instance, "xrCreateVulkanInstanceKHR",
                               (PFN_xrVoidFunction *) &CreateVulkanInstanceKHR);
  if (!_check_xr_result (res, "Failed to load xrCreateVulkanInstanceKHR."))
    {
      g_free (extension_names);
      g_slist_free_full (instance_ext_list_reduced, g_free);
      return FALSE;
    }

  VkResult vk_result;
  res = CreateVulkanInstanceKHR (self->instance, &xr_vk_instance_info, instance,
                                 &vk_result);

  g_free (extension_names);
  g_slist_free_full (instance_ext_list_reduced, g_free);

  if (!_check_xr_result (res, "Failed to create Vulkan instance!"))
    return FALSE;

  if (vk_result != VK_SUCCESS)
    {
      g_printerr ("Runtime failed to create Vulkan instance: %d\n", vk_result);
      return FALSE;
    }

  return TRUE;
}

static gboolean
_get_vk_physical_device2 (GxrContext       *self,
                          VkInstance        vk_instance,
                          VkPhysicalDevice *physical_device)
{
  PFN_xrGetVulkanGraphicsDevice2KHR fun = NULL;
  XrResult                          res;
  res = xrGetInstanceProcAddr (self->instance, "xrGetVulkanGraphicsDevice2KHR",
                               (PFN_xrVoidFunction *) &fun);

  if (!_check_xr_result (res, "Failed to load xrGetVulkanGraphicsDevice2KHR."))
    return FALSE;

  XrVulkanGraphicsDeviceGetInfoKHR info = {
    .type = XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR,
    .next = NULL,
    .systemId = self->system_id,
    .vulkanInstance = vk_instance,
  };

  res = fun (self->instance, &info, physical_device);

  if (!_check_xr_result (res, "Failed to get Vulkan graphics device."))
    return FALSE;

  return TRUE;
}

static gboolean
find_queue_index_for_flags (VkQueueFlagBits          flags,
                            VkQueueFlagBits          exclude_flags,
                            uint32_t                 num_queues,
                            VkQueueFamilyProperties *queue_family_props,
                            uint32_t                *out_index)
{
  uint32_t i = 0;
  for (i = 0; i < num_queues; i++)
    if (queue_family_props[i].queueFlags & flags
        && !(queue_family_props[i].queueFlags & exclude_flags))
      break;

  if (i >= num_queues)
    return FALSE;

  *out_index = i;
  return TRUE;
}

static gboolean
_find_queue_families (VkPhysicalDevice vk_physical_device,
                      uint32_t        *graphics_queue_index,
                      uint32_t        *transfer_queue_index)
{
  /* Find the first graphics queue */
  uint32_t num_queues = 0;
  vkGetPhysicalDeviceQueueFamilyProperties (vk_physical_device, &num_queues, 0);

  VkQueueFamilyProperties *queue_family_props
    = g_malloc (sizeof (VkQueueFamilyProperties) * num_queues);

  vkGetPhysicalDeviceQueueFamilyProperties (vk_physical_device, &num_queues,
                                            queue_family_props);

  if (num_queues == 0)
    {
      g_printerr ("Failed to get queue properties.\n");
      return FALSE;
    }

  *graphics_queue_index = UINT32_MAX;
  if (!find_queue_index_for_flags (VK_QUEUE_GRAPHICS_BIT, (VkQueueFlagBits) 0,
                                   num_queues, queue_family_props,
                                   graphics_queue_index))
    {
      g_printerr ("No graphics queue found\n");
      return FALSE;
    }

  *transfer_queue_index = UINT32_MAX;
  if (find_queue_index_for_flags (VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT,
                                  num_queues, queue_family_props,
                                  transfer_queue_index))
    {
      g_debug ("Got pure transfer queue");
    }
  else
    {
      g_debug ("No pure transfer queue found, trying all queues");
      if (find_queue_index_for_flags (VK_QUEUE_TRANSFER_BIT,
                                      (VkQueueFlagBits) 0, num_queues,
                                      queue_family_props, transfer_queue_index))
        {
          g_debug ("Got a transfer queue");
        }
      else
        {
          g_printerr ("No transfer queue found\n");
          return FALSE;
        }
    }

  g_free (queue_family_props);
  return TRUE;
}

static gboolean
_create_vk_device2 (GxrContext      *self,
                    VkPhysicalDevice physical_device,
                    GSList          *device_ext_list,
                    VkDevice        *vk_device,
                    uint32_t        *graphics_queue_index,
                    uint32_t        *transfer_queue_index)
{

  PFN_xrCreateVulkanDeviceKHR CreateVulkanDeviceKHR = NULL;
  XrResult                    res;
  res = xrGetInstanceProcAddr (self->instance, "xrCreateVulkanDeviceKHR",
                               (PFN_xrVoidFunction *) &CreateVulkanDeviceKHR);

  if (!_check_xr_result (res, "Failed to load xrCreateVulkanDeviceKHR."))
    return FALSE;

  if (!_find_queue_families (physical_device, graphics_queue_index,
                             transfer_queue_index))
    return FALSE;

  VkPhysicalDeviceFeatures physical_device_features;
  vkGetPhysicalDeviceFeatures (physical_device, &physical_device_features);

  GSList *device_ext_list_reduced = g_slist_copy_deep (device_ext_list,
                                                       _copy_str, NULL);
  _remove_unsupported_device_exts (physical_device, &device_ext_list_reduced);

  uint32_t     num_extensions = g_slist_length (device_ext_list_reduced);
  const char **extension_names = g_malloc (sizeof (char *) * num_extensions);

  uint32_t i = 0;
  for (GSList *l = device_ext_list_reduced; l; l = l->next)
    extension_names[i++] = l->data;

  // runtime will add extensions it requires
  VkDeviceCreateInfo device_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount = 2,
    .pQueueCreateInfos =
        (VkDeviceQueueCreateInfo[]) {
          {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = *graphics_queue_index,
            .queueCount = 1,
            .pQueuePriorities = (const float[]) { 1.0f },
          },
          {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = *transfer_queue_index,
            .queueCount = 1,
            .pQueuePriorities = (const float[]) { 0.8f },
          },
        },
    .pEnabledFeatures = &physical_device_features,
    .enabledExtensionCount = num_extensions,
    .ppEnabledExtensionNames = extension_names,
  };

#ifdef VK_API_VERSION_1_2
  VkPhysicalDeviceVulkan11Features enabled_vulkan11_features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
    .multiview = VK_TRUE,
  };
  if (self->desired_vk_version >= XR_MAKE_VERSION (1, 2, 0))
    device_info.pNext = &enabled_vulkan11_features;
#endif

  XrVulkanDeviceCreateInfoKHR info = {
    .type = XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR,
    .next = NULL,
    .systemId = self->system_id,
    .createFlags = 0,
    .pfnGetInstanceProcAddr = vkGetInstanceProcAddr,
    .vulkanPhysicalDevice = physical_device,
    .vulkanCreateInfo = &device_info,
    .vulkanAllocator = NULL,
  };

  VkResult vk_result;
  res = CreateVulkanDeviceKHR (self->instance, &info, vk_device, &vk_result);

  g_free (extension_names);
  g_slist_free_full (device_ext_list_reduced, g_free);

  if (!_check_xr_result (res, "Failed to create Vulkan graphics device."))
    return FALSE;

  if (vk_result != VK_SUCCESS)
    {
      g_printerr ("Runtime failed to create Vulkan device: %d\n", vk_result);
      return FALSE;
    }

  return TRUE;
}

static gboolean
init_vulkan_enable2 (GxrContext *self,
                     GSList     *instance_ext_list,
                     GSList     *device_ext_list)
{
  if (!_check_graphics_api_support (self))
    return FALSE;

  VkInstance vk_instance;
  if (!_create_vk_instance2 (self, instance_ext_list, &vk_instance))
    return FALSE;

  VkPhysicalDevice physical_device;
  if (!_get_vk_physical_device2 (self, vk_instance, &physical_device))
    return TRUE;

  VkDevice vk_device;
  uint32_t graphics_queue_index;
  uint32_t transfer_queue_index;
  if (!_create_vk_device2 (self, physical_device, device_ext_list, &vk_device,
                           &graphics_queue_index, &transfer_queue_index))
    return TRUE;

  self->gc = gulkan_context_new_from_vk (vk_instance, physical_device,
                                         vk_device, graphics_queue_index,
                                         transfer_queue_index);

  if (!self->gc)
    return FALSE;

  return TRUE;
}

static GxrContext *
_new (GSList  *instance_ext_list,
      GSList  *device_ext_list,
      char    *app_name,
      uint32_t app_version)
{
  GxrContext *self = (GxrContext *) g_object_new (GXR_TYPE_CONTEXT, 0);
  if (!self)
    {
      g_printerr ("Could not init gxr context.\n");
      return NULL;
    }

  if (!_init_runtime (self, app_name, app_version))
    {
      g_object_unref (self);
      g_printerr ("Could not init runtime.\n");
      return NULL;
    }

  // Always enable multiview extension.
  device_ext_list = g_slist_append (device_ext_list,
                                    g_strdup (VK_KHR_MULTIVIEW_EXTENSION_NAME));

  if (!init_vulkan_enable2 (self, instance_ext_list, device_ext_list))
    {
      g_free (self);
      return NULL;
    }

  if (!_init_session (self))
    {
      g_object_unref (self);
      g_printerr ("Could not init VR session.\n");
      return NULL;
    }

  g_mutex_init (&self->wait_frame_mutex);
  return self;
}

GxrContext *
gxr_context_new (char *app_name, uint32_t app_version)
{
  return gxr_context_new_from_vulkan_extensions (NULL, NULL, app_name,
                                                 app_version);
}

GxrContext *
gxr_context_new_from_vulkan_extensions (GSList  *instance_ext_list,
                                        GSList  *device_ext_list,
                                        char    *app_name,
                                        uint32_t app_version)
{
  return _new (instance_ext_list, device_ext_list, app_name, app_version);
}

static void
gxr_context_init (GxrContext *self)
{
  self->gc = NULL;
  self->device_manager = gxr_device_manager_new ();
  self->view_count = 0;
  self->views = NULL;
  self->predicted_display_time = 0;
  self->predicted_display_period = 0;
  self->framebuffers = NULL;

  self->swapchain[GxrSwapchainTypeColor].array_size = 2;
  self->swapchain[GxrSwapchainTypeColor].images = NULL;

  self->swapchain[GxrSwapchainTypeDepth].array_size = 2;
  self->swapchain[GxrSwapchainTypeDepth].images = NULL;

  self->desired_vk_version = XR_MAKE_VERSION (1, 2, 0);
}

static void
_cleanup_swapchain (GxrContext *self, struct GxrSwapchain *swapchain)
{
  (void) self;
  if (swapchain->handle != XR_NULL_HANDLE)
    xrDestroySwapchain (swapchain->handle);

  if (swapchain->images)
    g_free (swapchain->images);
}

static void
_cleanup (GxrContext *self)
{

  if (self->play_space)
    xrDestroySpace (self->play_space);
  if (self->session)
    xrDestroySession (self->session);
  if (self->instance)
    xrDestroyInstance (self->instance);

  g_free (self->configuration_views);

  g_free (self->views);
  g_free (self->projection_views);

  if (self->framebuffers)
    {
      for (uint32_t i = 0; i < self->swapchain[GxrSwapchainTypeColor].length;
           i++)
        {
          if (GULKAN_IS_FRAME_BUFFER (self->framebuffers[i]))
            g_object_unref (self->framebuffers[i]);
          else
            g_printerr ("Failed to release framebuffer %d\n", i);
        }
      g_free (self->framebuffers);
    }

  _cleanup_swapchain (self, &self->swapchain[GxrSwapchainTypeColor]);
  _cleanup_swapchain (self, &self->swapchain[GxrSwapchainTypeDepth]);
}

static void
gxr_context_finalize (GObject *gobject)
{
  GxrContext *self = GXR_CONTEXT (gobject);

  _cleanup (self);

  GulkanContext *gulkan = gxr_context_get_gulkan (GXR_CONTEXT (gobject));

  if (gulkan)
    g_object_unref (gulkan);

  if (self->device_manager != NULL)
    g_clear_object (&self->device_manager);

  /* child classes MUST destroy gulkan after this destructor finishes */

  g_debug ("destroyed up gxr context, bye");

  G_OBJECT_CLASS (gxr_context_parent_class)->finalize (gobject);
}

GulkanContext *
gxr_context_get_gulkan (GxrContext *self)
{
  return self->gc;
}

static gboolean
_space_location_valid (XrSpaceLocation *sl)
{
  return (sl->locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0
         && (sl->locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0;
}

static void
_get_model_matrix_from_pose (XrPosef *pose, graphene_matrix_t *mat)
{
  graphene_quaternion_t q;
  graphene_quaternion_init (&q, pose->orientation.x, pose->orientation.y,
                            pose->orientation.z, pose->orientation.w);

  graphene_matrix_t rotation;
  graphene_matrix_init_identity (&rotation);
  graphene_matrix_rotate_quaternion (&rotation, &q);

  graphene_point3d_t position = {
    pose->position.x,
    pose->position.y,
    pose->position.z,
  };

  graphene_matrix_t translation;
  graphene_matrix_init_translate (&translation, &position);

  graphene_matrix_multiply (&rotation, &translation, mat);
}

gboolean
gxr_context_get_head_pose (GxrContext *self, graphene_matrix_t *pose)
{
  XrSpaceLocation space_location = {
    .type = XR_TYPE_SPACE_LOCATION,
  };

  XrResult result = xrLocateSpace (self->view_space, self->play_space,
                                   self->predicted_display_time,
                                   &space_location);
  _check_xr_result (result, "Failed to locate head space.");

  gboolean valid = _space_location_valid (&space_location);
  if (!valid)
    {
      g_printerr ("Could not get valid head pose.\n");
      graphene_matrix_init_identity (pose);
      return FALSE;
    }

  _get_model_matrix_from_pose (&space_location.pose, pose);
  return TRUE;
}

#define PI 3.1415926535f
#define RAD_TO_DEG(x) ((x) *360.0f / (2.0f * PI))

void
gxr_context_get_frustum_angles (GxrContext *self,
                                GxrEye      eye,
                                float      *left,
                                float      *right,
                                float      *top,
                                float      *bottom)
{
  *left = RAD_TO_DEG (self->views[eye].fov.angleLeft);
  *right = RAD_TO_DEG (self->views[eye].fov.angleRight);
  *top = RAD_TO_DEG (self->views[eye].fov.angleUp);
  *bottom = RAD_TO_DEG (self->views[eye].fov.angleDown);

  // g_debug ("Fov angles L %f R %f T %f B %f", *left, *right, *top, *bottom);
}

static void
_handle_visibility_changed (GxrContext *self, XrEventDataBuffer *runtimeEvent)
{
  XrEventDataMainSessionVisibilityChangedEXTX *event
    = (XrEventDataMainSessionVisibilityChangedEXTX *) runtimeEvent;

  g_debug ("Event: main session visibility now: %d", event->visible);

  GxrOverlayEvent overlay_event = {
    .main_session_visible = event->visible,
  };
  g_signal_emit (self, context_signals[OVERLAY_EVENT], 0, &overlay_event);
}

static void
_handle_state_changed (GxrContext *self, XrEventDataBuffer *runtimeEvent)
{
  XrEventDataSessionStateChanged *event = (XrEventDataSessionStateChanged *)
    runtimeEvent;

  self->session_state = event->state;
  g_debug ("EVENT: session state changed to %d", event->state);

  /*
   * react to session state changes, see OpenXR spec 9.3 diagram. What we need
   * to react to:
   *
   * * READY -> xrBeginSession STOPPING -> xrEndSession (note that the same
   * session can be restarted)
   * * EXITING -> xrDestroySession (EXITING only happens after we went through
   * STOPPING and called xrEndSession)
   *
   * After exiting it is still possible to create a new session but we don't do
   * that here.
   *
   * * IDLE -> don't run render loop, but keep polling for events
   * * SYNCHRONIZED, VISIBLE, FOCUSED -> run render loop
   */
  gboolean should_submit_frames = FALSE;
  switch (event->state)
    {
      // skip render loop, keep polling
      case XR_SESSION_STATE_MAX_ENUM: // must be a bug
      case XR_SESSION_STATE_IDLE:
      case XR_SESSION_STATE_UNKNOWN:
        break; // state handling switch

      // do nothing, run render loop normally
      case XR_SESSION_STATE_FOCUSED:
      case XR_SESSION_STATE_SYNCHRONIZED:
      case XR_SESSION_STATE_VISIBLE:
        should_submit_frames = TRUE;
        break; // state handling switch

      // begin session and then run render loop
      case XR_SESSION_STATE_READY:
        {
          // start session only if it is not running, i.e. not when we already
          // called xrBeginSession but the runtime did not switch to the next
          // state yet
          if (!self->session_running)
            {
              if (!_begin_session (self))
                g_printerr ("Failed to begin session\n");
            }
          should_submit_frames = TRUE;
          break; // state handling switch
        }

      // end session, skip render loop, keep polling for next state change
      case XR_SESSION_STATE_STOPPING:
        {
          // end session only if it is running, i.e. not when we already called
          // xrEndSession but the runtime did not switch to the next state yet
          if (self->session_running)
            {
              if (!_end_session (self))
                g_printerr ("Failed to end session\n");
            }
          break; // state handling switch
        }

      // destroy session, skip render loop, exit render loop and quit
      case XR_SESSION_STATE_LOSS_PENDING:
      case XR_SESSION_STATE_EXITING:
        if (self->session)
          {
            if (!_destroy_session (self))
              g_printerr ("Failed to destroy session\n");

            GxrStateChangeEvent state_change_event = {
              .state_change = GXR_STATE_SHUTDOWN,
            };
            g_debug ("Event: state is EXITING, sending VR_QUIT_SHUTDOWN");
            g_signal_emit (self, context_signals[STATE_CHANGE_EVENT], 0,
                           &state_change_event);
          }
        break; // state handling switch
    }

  if (!self->should_submit_frames && should_submit_frames)
    {
      GxrStateChangeEvent state_change_event = {
        .state_change = GXR_STATE_FRAMECYCLE_START,
      };
      g_debug ("Event: start frame cycle");
      g_signal_emit (self, context_signals[STATE_CHANGE_EVENT], 0,
                     &state_change_event);
    }
  else if (self->should_submit_frames && !should_submit_frames)
    {
      GxrStateChangeEvent state_change_event = {
        .state_change = GXR_STATE_FRAMECYCLE_STOP,
      };
      g_debug ("Event: stop frame cycle");
      g_signal_emit (self, context_signals[STATE_CHANGE_EVENT], 0,
                     &state_change_event);
    }

  self->should_submit_frames = should_submit_frames;
}

static void
_handle_instance_loss (GxrContext *self)
{
  GxrStateChangeEvent event = {
    .state_change = GXR_STATE_SHUTDOWN,
  };
  g_debug ("Event: instance loss pending, sending VR_QUIT_SHUTDOWN");
  g_signal_emit (self, context_signals[STATE_CHANGE_EVENT], 0, &event);
}

static void
_handle_interaction_profile_changed (GxrContext *self)
{
  g_print ("Event: STUB: interaction profile changed\n");

  XrInteractionProfileState state = {
    .type = XR_TYPE_INTERACTION_PROFILE_STATE,
  };

  char  *hand_str[2] = {"/user/hand/left", "/user/hand/right"};
  XrPath hand_paths[2];
  xrStringToPath (self->instance, hand_str[0], &hand_paths[0]);
  xrStringToPath (self->instance, hand_str[1], &hand_paths[1]);
  for (int i = 0; i < NUM_CONTROLLERS; i++)
    {
      XrResult res = xrGetCurrentInteractionProfile (self->session,
                                                     hand_paths[i], &state);
      if (!_check_xr_result (res, "Failed to get interaction profile for %s",
                             hand_str[i]))
        continue;

      XrPath prof = state.interactionProfile;

      if (prof == XR_NULL_PATH)
        {
          // perhaps no controller is present
          g_debug ("Event: Interaction profile on %s: [none]", hand_str[i]);
          continue;
        }

      uint32_t strl;
      char     profile_str[XR_MAX_PATH_LENGTH];
      res = xrPathToString (self->instance, prof, XR_MAX_PATH_LENGTH, &strl,
                            profile_str);
      if (!_check_xr_result (res,
                             "Failed to get interaction profile path str for "
                             "%s",
                             hand_str[i]))
        continue;

      g_debug ("Event: Interaction profile on %s: %s", hand_str[i],
               profile_str);
    }
}

void
gxr_context_poll_events (GxrContext *self)
{
  XrEventDataBuffer runtimeEvent = {
    .type = XR_TYPE_EVENT_DATA_BUFFER,
  };

  XrResult pollResult;
  while (TRUE)
    {
      runtimeEvent.type = XR_TYPE_EVENT_DATA_BUFFER;
      pollResult = xrPollEvent (self->instance, &runtimeEvent);
      if (pollResult != XR_SUCCESS)
        break;

      switch (runtimeEvent.type)
        {
          case XR_TYPE_EVENT_DATA_EVENTS_LOST:
            /* We didnt poll enough */
            g_printerr ("Event: Events lost\n");
            break;
          case XR_TYPE_EVENT_DATA_MAIN_SESSION_VISIBILITY_CHANGED_EXTX:
            _handle_visibility_changed (self, &runtimeEvent);
            break;
          case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT:
            g_debug ("Event: STUB: perf settings");
            break;
          case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR:
            g_debug ("Event: STUB: visibility mask changed");
            break;
          case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
            _handle_state_changed (self, &runtimeEvent);
            break;
          case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
            _handle_instance_loss (self);
            break;
          case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
            _handle_interaction_profile_changed (self);
            break;
          case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
            g_debug ("Event: STUB: reference space change pending\n");
            break;
          default:
            {
              char buffer[XR_MAX_STRUCTURE_NAME_SIZE];
              xrStructureTypeToString (self->instance, runtimeEvent.type,
                                       buffer);
              g_print ("Event: Unhandled event type %s (%d)\n", buffer,
                       runtimeEvent.type);
              break;
            }
        }
    }

  if (pollResult != XR_EVENT_UNAVAILABLE)
    {
      g_printerr ("Failed to poll events!\n");
    }
}

gboolean
gxr_context_init_framebuffers (GxrContext           *self,
                               VkExtent2D            extent,
                               VkSampleCountFlagBits sample_count,
                               GulkanRenderPass    **render_pass)
{
  GulkanContext *gc = gxr_context_get_gulkan (self);
  GulkanDevice  *device = gulkan_context_get_device (gc);

  self->framebuffer_extent = extent;
  self->framebuffer_sample_count = sample_count;

  VkFormat format = self->swapchain[GxrSwapchainTypeColor].format;
  VkFormat depth_format = self->swapchain[GxrSwapchainTypeDepth].format;
  *render_pass
    = gulkan_render_pass_new_multiview (device, sample_count, format,
                                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                        TRUE, depth_format);
  if (!*render_pass)
    {
      g_printerr ("Could not init render pass.\n");
      return FALSE;
    }

  g_debug ("Creating %d framebuffers",
           self->swapchain[GxrSwapchainTypeColor].length);

  self->framebuffers = g_malloc (sizeof (GulkanFrameBuffer *)
                                 * self->swapchain[GxrSwapchainTypeColor]
                                     .length);
  for (uint32_t i = 0; i < self->swapchain[GxrSwapchainTypeColor].length; i++)
    {
      VkImage color_image = self->swapchain[GxrSwapchainTypeColor]
                              .images[i]
                              .image;
      VkImage depth_image = self->swapchain[GxrSwapchainTypeDepth]
                              .images[i]
                              .image;

      self->framebuffers[i]
        = gulkan_frame_buffer_new_from_image_with_depth (device, *render_pass,
                                                         color_image, extent,
                                                         sample_count, format,
                                                         self->view_count,
                                                         depth_image,
                                                         depth_format);

      if (!GULKAN_IS_FRAME_BUFFER (self->framebuffers[i]))
        {
          g_printerr ("Could not initialize frambuffer.");
          return FALSE;
        }
    }

  return TRUE;
}

static void
_get_projection_matrix_from_fov (const XrFovf       fov,
                                 const float        near_z,
                                 const float        far_z,
                                 graphene_matrix_t *mat)
{
  const float tan_left = tanf (fov.angleLeft);
  const float tan_right = tanf (fov.angleRight);

  const float tan_down = tanf (fov.angleDown);
  const float tan_up = tanf (fov.angleUp);

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

void
gxr_context_get_projection (GxrContext        *self,
                            GxrEye             eye,
                            float              near,
                            float              far,
                            graphene_matrix_t *mat)
{
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
_get_view_matrix_from_pose (XrPosef *pose, graphene_matrix_t *mat)
{
  graphene_quaternion_t q;
  graphene_quaternion_init (&q, pose->orientation.x, pose->orientation.y,
                            pose->orientation.z, pose->orientation.w);

  graphene_matrix_t rotation;
  graphene_matrix_init_identity (&rotation);
  graphene_matrix_rotate_quaternion (&rotation, &q);

  graphene_point3d_t position = {
    pose->position.x,
    pose->position.y,
    pose->position.z,
  };

  graphene_matrix_t translation;
  graphene_matrix_init_translate (&translation, &position);

  graphene_matrix_t view;
  graphene_matrix_multiply (&rotation, &translation, &view);

  graphene_matrix_inverse (&view, mat);
}

void
gxr_context_get_view (GxrContext *self, GxrEye eye, graphene_matrix_t *mat)
{
  if (self->views == NULL)
    {
      g_warning ("get_view needs to be called between begin and end frame.\n");
      graphene_matrix_init_identity (mat);
      return;
    }
  _get_view_matrix_from_pose (&self->views[eye].pose, mat);
}

void
gxr_context_get_eye_position (GxrContext *self, GxrEye eye, graphene_vec3_t *v)
{
  graphene_vec3_init (v, self->views[eye].pose.position.x,
                      self->views[eye].pose.position.y,
                      self->views[eye].pose.position.z);
}

static gboolean
_acquire_and_wait (GxrContext *self, struct GxrSwapchain *swapchain)
{
  (void) self;
  XrResult                    result;
  XrSwapchainImageAcquireInfo swapchainImageAcquireInfo = {
    .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO,
  };

  result = xrAcquireSwapchainImage (swapchain->handle,
                                    &swapchainImageAcquireInfo,
                                    &swapchain->buffer_index);
  if (!_check_xr_result (result, "failed to acquire swapchain image!"))
    return FALSE;

#define NANO_TO_MILLI 1000000

  XrSwapchainImageWaitInfo swapchainImageWaitInfo = {
    .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
    .timeout = 100 * NANO_TO_MILLI,
  };
  while ((result = xrWaitSwapchainImage (swapchain->handle,
                                         &swapchainImageWaitInfo))
         == XR_TIMEOUT_EXPIRED)
    {
      g_warning ("Waiting for swapchain timed out after 100 ms...");
    }

  if (!_check_xr_result (result, "failed to wait for swapchain image!"))
    return FALSE;

  return TRUE;
}

gboolean
gxr_context_wait_frame (GxrContext *self)
{
  XrResult result;

  XrFrameState frame_state = {
    .type = XR_TYPE_FRAME_STATE,
  };
  XrFrameWaitInfo frameWaitInfo = {
    .type = XR_TYPE_FRAME_WAIT_INFO,
  };
  result = xrWaitFrame (self->session, &frameWaitInfo, &frame_state);
  if (!_check_xr_result (result,
                         "xrWaitFrame() was not successful, exiting..."))
    return FALSE;

  if (!_acquire_and_wait (self, &self->swapchain[GxrSwapchainTypeColor]))
    {
      g_printerr ("Failed to acquire color image");
      return FALSE;
    }

  if (!_acquire_and_wait (self, &self->swapchain[GxrSwapchainTypeDepth]))
    {
      g_printerr ("Failed to acquire depth image");
      return FALSE;
    }

  g_mutex_lock (&self->wait_frame_mutex);

  if (!self->should_render && frame_state.shouldRender)
    {
      GxrStateChangeEvent state_change_event = {
        .state_change = GXR_STATE_RENDERING_START,
      };
      g_debug ("Event: start rendering");
      g_signal_emit (self, context_signals[STATE_CHANGE_EVENT], 0,
                     &state_change_event);
    }
  else if (self->should_render && !frame_state.shouldRender)
    {
      GxrStateChangeEvent state_change_event = {
        .state_change = GXR_STATE_RENDERING_STOP,
      };
      g_debug ("Event: stop rendering");
      g_signal_emit (self, context_signals[STATE_CHANGE_EVENT], 0,
                     &state_change_event);
    }

  self->should_render = frame_state.shouldRender == XR_TRUE;

  self->predicted_display_time = frame_state.predictedDisplayTime;
  self->predicted_display_period = frame_state.predictedDisplayPeriod;

  g_mutex_unlock (&self->wait_frame_mutex);

  return TRUE;
}

static gboolean
_begin_frame (GxrContext *self)
{
  if (!self->session_running)
    {
      g_printerr ("Can't begin frame with no session running\n");
      return FALSE;
    }

  if (self->session_state == XR_SESSION_STATE_EXITING
      || self->session_state == XR_SESSION_STATE_LOSS_PENDING
      || self->session_state == XR_SESSION_STATE_STOPPING)
    return FALSE;

  XrResult result;

  g_mutex_lock (&self->wait_frame_mutex);
  XrTime predicted_display_time = self->predicted_display_time;
  g_mutex_unlock (&self->wait_frame_mutex);

  // --- Create projection matrices and view matrices for each eye
  XrViewLocateInfo viewLocateInfo = {
    .type = XR_TYPE_VIEW_LOCATE_INFO,
    .displayTime = predicted_display_time,
    .space = self->play_space,
    .viewConfigurationType = self->view_config_type,
  };

  XrViewState viewState = {
    .type = XR_TYPE_VIEW_STATE,
  };
  uint32_t viewCountOutput;
  result = xrLocateViews (self->session, &viewLocateInfo, &viewState,
                          self->view_count, &viewCountOutput, self->views);
  if (!_check_xr_result (result, "Could not locate views"))
    return FALSE;

  // --- Begin frame
  XrFrameBeginInfo frameBeginInfo = {
    .type = XR_TYPE_FRAME_BEGIN_INFO,
  };

  result = xrBeginFrame (self->session, &frameBeginInfo);
  if (!_check_xr_result (result, "failed to begin frame!"))
    return FALSE;

  self->have_valid_pose = (viewState.viewStateFlags
                           & XR_VIEW_STATE_ORIENTATION_VALID_BIT)
                            != 0
                          && (viewState.viewStateFlags
                              & XR_VIEW_STATE_POSITION_VALID_BIT)
                               != 0;

  return TRUE;
}

gboolean
gxr_context_begin_frame (GxrContext *self)
{
  if (!_begin_frame (self))
    {
      g_printerr ("Could not begin XR frame.\n");
      return FALSE;
    }

  for (uint32_t i = 0; i < 2; i++)
    {
      self->projection_views[i].pose = self->views[i].pose;
      self->projection_views[i].fov = self->views[i].fov;
      self->projection_views[i].subImage.imageArrayIndex = i;
    }

  /* TODO: update poses, so GxrContext can update them for DeviceManager.
   * device poses are used for device model rendering, which is not in the
   * OpenXR spec yet.
   * Interaction is done using aim and grip pose actions instead.
   */
  /* TODO: This can be probably removed */
  GxrPose poses[GXR_DEVICE_INDEX_MAX];
  for (int i = 0; i < GXR_DEVICE_INDEX_MAX; i++)
    {
      poses[i].is_valid = FALSE;
    }
  GxrDeviceManager *dm = gxr_context_get_device_manager (self);
  gxr_device_manager_update_poses (dm, poses);

  return TRUE;
}

static gboolean
_end_frame (GxrContext *self)
{
  if (!self->session_running)
    {
      g_printerr ("Can't end frame with no session running\n");
      return FALSE;
    }

  XrResult result;

  const XrCompositionLayerBaseHeader *layers[1];
  uint32_t                            layer_count = 0;

  g_mutex_lock (&self->wait_frame_mutex);

  // if we end up here but shouldn't render, the app probably hasn't rendered
  if (self->should_render)
    {
      // for submiting projection layers we need a valid pose from xrLocateViews
      if (self->have_valid_pose)
        {
          layers[layer_count++] = (const XrCompositionLayerBaseHeader *) &self
                                    ->projection_layer;
        }
    }

  XrFrameEndInfo frame_end_info = {
    .type = XR_TYPE_FRAME_END_INFO,
    .displayTime = self->predicted_display_time,
    .layerCount = layer_count,
    .layers = layers,
    .environmentBlendMode = self->blend_mode,
  };
  result = xrEndFrame (self->session, &frame_end_info);
  if (!_check_xr_result (result, "failed to end frame!"))
    return FALSE;

  g_mutex_unlock (&self->wait_frame_mutex);

  return TRUE;
}

static gboolean
_release_swapchain (GxrContext *self, struct GxrSwapchain *swapchain)
{
  (void) self;
  XrSwapchainImageReleaseInfo swapchainImageReleaseInfo = {
    .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
  };
  XrResult result = xrReleaseSwapchainImage (swapchain->handle,
                                             &swapchainImageReleaseInfo);
  if (!_check_xr_result (result, "failed to release swapchain image!"))
    return FALSE;

  return TRUE;
}

gboolean
gxr_context_end_frame (GxrContext *self,
                       float       near_z,
                       float       far_z,
                       float       min_depth,
                       float       max_depth)
{
  if (!_release_swapchain (self, &self->swapchain[GxrSwapchainTypeColor]))
    {
      g_printerr ("Could not release xr swapchain\n");
      return FALSE;
    }

  if (!_release_swapchain (self, &self->swapchain[GxrSwapchainTypeDepth]))
    {
      g_printerr ("Could not release xr swapchain\n");
      return FALSE;
    }

  if (self->extensions.depth)
    {
      for (uint32_t i = 0; i < self->view_count; i++)
        {
          self->depth_infos[i].nearZ = near_z;
          self->depth_infos[i].farZ = far_z;
          self->depth_infos[i].minDepth = min_depth;
          self->depth_infos[i].maxDepth = max_depth;
        }
    }

  if (!_end_frame (self))
    {
      g_printerr ("Could not end xr frame\n");
    }

  return TRUE;
}

void
gxr_context_request_quit (GxrContext *self)
{
  /* _poll_event will send quit event once session is in STOPPING state */
  xrRequestExitSession (self->session);
}

gboolean
gxr_context_get_runtime_instance_extensions (GxrContext *self,
                                             GSList    **out_list)
{
  PFN_xrGetVulkanInstanceExtensionsKHR GetVulkanInstanceExtensionsKHR = NULL;
  XrResult                             res;
  res = xrGetInstanceProcAddr (self->instance,
                               "xrGetVulkanInstanceExtensionsKHR",
                               (PFN_xrVoidFunction
                                  *) (&GetVulkanInstanceExtensionsKHR));
  if (!_check_xr_result (res, "Failed to retrieve "
                              "xrGetVulkanGraphicsRequirements2KHR pointer!"))
    return FALSE;

  uint32_t ext_str_len = 0;
  res = GetVulkanInstanceExtensionsKHR (self->instance, self->system_id, 0,
                                        &ext_str_len, NULL);
  if (!_check_xr_result (res, "Failed to get Vulkan instance extension string "
                              "size!"))
    return FALSE;

  gchar *ext_str = g_malloc (sizeof (gchar) * ext_str_len);

  res = GetVulkanInstanceExtensionsKHR (self->instance, self->system_id,
                                        ext_str_len, &ext_str_len, ext_str);
  if (!_check_xr_result (res,
                         "Failed to get Vulkan instance extension string!"))
    {
      g_free (ext_str);
      return FALSE;
    }

  g_debug ("runtime instance ext str: %s", ext_str);

  gchar **ext_str_split = g_strsplit (ext_str, " ", 0);
  for (gchar **token = ext_str_split; *token; token++)
    *out_list = g_slist_append (*out_list, g_strdup (*token));

  g_strfreev (ext_str_split);
  g_free (ext_str);
  return TRUE;
}

gboolean
gxr_context_get_runtime_device_extensions (GxrContext *self, GSList **out_list)
{
  PFN_xrGetVulkanDeviceExtensionsKHR GetVulkanDeviceExtensionsKHR = NULL;
  XrResult                           res;
  res = xrGetInstanceProcAddr (self->instance, "xrGetVulkanDeviceExtensionsKHR",
                               (PFN_xrVoidFunction
                                  *) (&GetVulkanDeviceExtensionsKHR));
  if (!_check_xr_result (res, "Failed to retrieve "
                              "xrGetVulkanGraphicsRequirements2KHR pointer!"))
    return FALSE;

  uint32_t ext_str_len = 0;
  res = GetVulkanDeviceExtensionsKHR (self->instance, self->system_id, 0,
                                      &ext_str_len, NULL);
  if (!_check_xr_result (res,
                         "Failed to get Vulkan device extension string size!"))
    return FALSE;

  gchar *ext_str = g_malloc (sizeof (gchar) * ext_str_len);

  res = GetVulkanDeviceExtensionsKHR (self->instance, self->system_id,
                                      ext_str_len, &ext_str_len, ext_str);
  if (!_check_xr_result (res, "Failed to get Vulkan device extension string!"))
    {
      g_free (ext_str);
      return FALSE;
    }

  g_debug ("runtime device ext str: %s", ext_str);

  gchar **ext_str_split = g_strsplit (ext_str, " ", 0);
  for (gchar **token = ext_str_split; *token; token++)
    *out_list = g_slist_append (*out_list, g_strdup (*token));

  g_strfreev (ext_str_split);
  g_free (ext_str);
  return TRUE;
  return TRUE;
}

GxrDeviceManager *
gxr_context_get_device_manager (GxrContext *self)
{
  return self->device_manager;
}

uint32_t
gxr_context_get_swapchain_length (GxrContext *self)
{
  return self->swapchain[GxrSwapchainTypeColor].length;
}

GulkanFrameBuffer *
gxr_context_get_acquired_framebuffer (GxrContext *self)
{
  g_mutex_lock (&self->wait_frame_mutex);
  GulkanFrameBuffer *fb = GULKAN_FRAME_BUFFER (self->framebuffers
                                                 [self
                                                    ->swapchain
                                                      [GxrSwapchainTypeColor]
                                                    .buffer_index]);
  g_mutex_unlock (&self->wait_frame_mutex);
  return fb;
}

GulkanFrameBuffer *
gxr_context_get_framebuffer_at (GxrContext *self, uint32_t i)
{
  GulkanFrameBuffer *fb = GULKAN_FRAME_BUFFER (self->framebuffers[i]);
  return fb;
}

uint32_t
gxr_context_get_buffer_index (GxrContext *self)
{
  g_mutex_lock (&self->wait_frame_mutex);
  uint32_t buffer_index = self->swapchain[GxrSwapchainTypeColor].buffer_index;
  g_mutex_unlock (&self->wait_frame_mutex);
  return buffer_index;
}

XrSessionState
gxr_context_get_session_state (GxrContext *self)
{
  return self->session_state;
}

XrTime
gxr_context_get_predicted_display_time (GxrContext *self)
{
  g_mutex_lock (&self->wait_frame_mutex);
  XrTime time = self->predicted_display_time;
  g_mutex_unlock (&self->wait_frame_mutex);
  return time;
}

XrInstance
gxr_context_get_openxr_instance (GxrContext *self)
{
  return self->instance;
}

XrSession
gxr_context_get_openxr_session (GxrContext *self)
{
  return self->session;
}

XrSpace
gxr_context_get_tracked_space (GxrContext *self)
{
  return self->play_space;
}

VkExtent2D
gxr_context_get_swapchain_extent (GxrContext *self, uint32_t view_index)
{
  VkExtent2D extent = {
    .width = self->configuration_views[view_index].recommendedImageRectWidth,
    .height = self->configuration_views[view_index].recommendedImageRectHeight,
  };
  return extent;
}

static GxrAction *
_find_openxr_action_from_url (GxrActionSet **sets, uint32_t count, gchar *url)
{
  for (uint32_t i = 0; i < count; i++)
    {
      GSList *actions = gxr_action_set_get_actions (GXR_ACTION_SET (sets[i]));
      for (GSList *l = actions; l != NULL; l = l->next)
        {
          GxrAction *action = GXR_ACTION (l->data);
          gchar     *action_url = gxr_action_get_url (action);
          if (g_strcmp0 (action_url, url) == 0)
            return action;
        }
    }
  g_debug ("Skipping action %s not connected by application", url);
  return NULL;
}

static uint32_t
_count_input_paths (GxrBindingManifest *binding_manifest)
{
  uint32_t num = 0;
  for (GSList *l = binding_manifest->gxr_bindings; l; l = l->next)
    {
      GxrBinding *binding = l->data;
      num += g_slist_length (binding->input_paths);
    }
  return num;
}

static void
_update_controllers (GxrActionSet *self)
{
  GSList *actions = gxr_action_set_get_actions (GXR_ACTION_SET (self));
  for (GSList *l = actions; l != NULL; l = l->next)
    {
      GxrAction *action = GXR_ACTION (l->data);
      gxr_action_update_controllers (action);
    }
}

static gchar *
_component_to_str (GxrBindingComponent c)
{
  switch (c)
    {
      case GXR_BINDING_COMPONENT_CLICK:
        return "click";
      case GXR_BINDING_COMPONENT_PULL:
        return "value";
      case GXR_BINDING_COMPONENT_TOUCH:
        return "touch";
      case GXR_BINDING_COMPONENT_FORCE:
        return "force";
      case GXR_BINDING_COMPONENT_POSITION:
        /* use .../trackpad instead of .../trackpad/x etc. */
        return NULL;
      default:
        return NULL;
    }
}

static gboolean
_suggest_for_interaction_profile (GxrActionSet      **sets,
                                  uint32_t            count,
                                  XrInstance          instance,
                                  GxrBindingManifest *binding_manifest)
{
  uint32_t num_bindings = _count_input_paths (binding_manifest);

  XrActionSuggestedBinding *suggested_bindings
    = g_malloc (sizeof (XrActionSuggestedBinding)
                * (unsigned long) num_bindings);

  uint32_t num_suggestion = 0;
  for (GSList *l = binding_manifest->gxr_bindings; l != NULL; l = l->next)
    {
      GxrBinding *binding = l->data;
      gchar      *action_url = binding->action->name;
      GxrAction  *action = _find_openxr_action_from_url (sets, count,
                                                         action_url);
      if (!action)
        continue;

      XrAction handle = gxr_action_get_handle (action);
      char    *url = gxr_action_get_url (action);

      g_debug ("Action %s has %d inputs", url,
               g_slist_length (binding->input_paths));

      for (GSList *m = binding->input_paths; m; m = m->next)
        {
          GxrBindingPath *input_path = m->data;

          gchar *component_str = _component_to_str (input_path->component);

          GString *full_path = g_string_new ("");
          if (component_str)
            g_string_printf (full_path, "%s/%s", input_path->path,
                             component_str);
          else
            g_string_append (full_path, input_path->path);

          g_debug ("\t%s ", full_path->str);

          XrPath component_path;
          xrStringToPath (instance, full_path->str, &component_path);

          suggested_bindings[num_suggestion].action = handle;
          suggested_bindings[num_suggestion].binding = component_path;

          num_suggestion++;

          g_string_free (full_path, TRUE);
        }
    }

  g_debug ("Suggested %d/%d bindings!", num_suggestion, num_bindings);
  XrPath profile_path;
  xrStringToPath (instance, binding_manifest->interaction_profile,
                  &profile_path);

  const XrInteractionProfileSuggestedBinding suggestion_info = {
    .type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING,
    .next = NULL,
    .interactionProfile = profile_path,
    .countSuggestedBindings = num_suggestion,
    .suggestedBindings = suggested_bindings,
  };

  XrResult result = xrSuggestInteractionProfileBindings (instance,
                                                         &suggestion_info);
  g_free (suggested_bindings);
  if (result != XR_SUCCESS)
    {
      char buffer[XR_MAX_RESULT_STRING_SIZE];
      xrResultToString (instance, result, buffer);
      g_printerr ("ERROR: Suggesting actions for profile %s: %s\n",
                  binding_manifest->interaction_profile, buffer);
      return FALSE;
    }

  return TRUE;
}

gboolean
gxr_context_attach_action_sets (GxrContext    *self,
                                GxrActionSet **sets,
                                uint32_t       count)
{
  for (uint32_t i = 0; i < count; i++)
    {
      uint32_t j;
      for (j = 0; j < i; j++)
        {
          GxrManifest *manifest_i = gxr_action_set_get_manifest (sets[i]);
          GxrManifest *manifest_j = gxr_action_set_get_manifest (sets[i]);
          if (manifest_i == manifest_j)
            break;
        }
      if (i == j)
        {
          // do for every unique manifest
          GxrManifest *manifest = gxr_action_set_get_manifest (sets[i]);
          GSList      *binding_manifests
            = gxr_manifest_get_binding_manifests (manifest);

          for (GSList *l = binding_manifests; l; l = l->next)
            {
              GxrBindingManifest *binding_manifest = l->data;

              g_debug ("===");
              g_debug ("Suggesting for profile %s",
                       binding_manifest->interaction_profile);

              _suggest_for_interaction_profile (sets, count, self->instance,
                                                binding_manifest);
            }
        }
    }

  XrActionSet *handles = g_malloc (sizeof (XrActionSet) * count);
  for (uint32_t i = 0; i < count; i++)
    {
      handles[i] = gxr_action_set_get_handle (sets[i]);
    }

  XrSessionActionSetsAttachInfo attachInfo = {
    .type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO,
    .next = NULL,
    .countActionSets = count,
    .actionSets = handles,
  };

  XrResult result = xrAttachSessionActionSets (self->session, &attachInfo);
  g_free (handles);
  if (result != XR_SUCCESS)
    {
      char buffer[XR_MAX_RESULT_STRING_SIZE];
      xrResultToString (self->instance, result, buffer);
      g_printerr ("ERROR: attaching action set: %s\n", buffer);
      return FALSE;
    }

  g_debug ("Attached %d action sets", count);
  for (uint32_t i = 0; i < count; i++)
    {
      _update_controllers (sets[i]);
    }
  g_debug ("Updating controllers based on actions");

  return TRUE;
}

gboolean
gxr_context_is_environment_opaque (GxrContext *self)
{
  return self->blend_mode == XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
}
