/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-context-private.h"

#include <openxr/openxr_reflection.h>

#include "gxr-controller.h"
#include "gxr-device-manager.h"
#include "gxr-version.h"
#include "gxr-manifest.h"

// TODO: Do not hardcode this
#define NUM_CONTROLLERS 2

struct _GxrContext
{
  GObject parent;
  GulkanClient *gc;

  GxrDeviceManager *device_manager;

  XrInstance instance;
  XrSession session;
  XrReferenceSpaceType play_space_type;
  XrSpace play_space;
  XrSpace view_space;
  XrSystemId system_id;
  XrViewConfigurationType view_config_type;

  /* One array per eye */
  XrSwapchain *swapchains;
  XrSwapchainImageVulkanKHR **images;
  /* last acquired swapchain image index per swapchain */
  uint32_t *buffer_index;
  /* for each view */
  uint32_t *swapchain_length;

  /* 1 framebuffer for each swapchain image, for each swapchain (1 per view) */
  GulkanFrameBuffer  ***framebuffer;
  VkExtent2D            framebuffer_extent;
  VkSampleCountFlagBits framebuffer_sample_count;

  XrCompositionLayerProjectionView* projection_views;
  XrViewConfigurationView* configuration_views;

  XrGraphicsBindingVulkanKHR graphics_binding;


  uint32_t view_count;

  XrSessionState session_state;
  gboolean should_render;
  gboolean have_valid_pose;
  gboolean is_stopping;

  XrCompositionLayerProjection projection_layer;

  volatile XrTime predicted_display_time;
  volatile XrDuration predicted_display_period;

  XrView* views;

  int64_t swapchain_format;

  GxrManifest *manifest;
};

G_DEFINE_TYPE (GxrContext, gxr_context, G_TYPE_OBJECT)

enum {
  KEYBOARD_PRESS_EVENT,
  KEYBOARD_CLOSE_EVENT,
  QUIT_EVENT,
  DEVICE_UPDATE_EVENT,
  BINDING_LOADED,
  BINDINGS_UPDATE,
  ACTIONSET_UPDATE,
  LAST_SIGNAL
};

static guint context_signals[LAST_SIGNAL] = { 0 };

static void
gxr_context_finalize (GObject *gobject);

static void
gxr_context_class_init (GxrContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gxr_context_finalize;

  context_signals[KEYBOARD_PRESS_EVENT] =
  g_signal_new ("keyboard-press-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL, G_TYPE_NONE,
                1, G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE);

  context_signals[KEYBOARD_CLOSE_EVENT] =
  g_signal_new ("keyboard-close-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  context_signals[QUIT_EVENT] =
  g_signal_new ("quit-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL, G_TYPE_NONE,
                1, G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE);

  context_signals[DEVICE_UPDATE_EVENT] =
  g_signal_new ("device-update-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL, G_TYPE_NONE,
                1, G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE);

  context_signals[BINDINGS_UPDATE] =
  g_signal_new ("bindings-update-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  context_signals[BINDING_LOADED] =
  g_signal_new ("binding-loaded-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  context_signals[ACTIONSET_UPDATE] =
  g_signal_new ("action-set-update-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, NULL, G_TYPE_NONE, 0);
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

#define BUF_LEN 1024
static gboolean
_check_xr_result (XrResult result, const char* format, ...)
{
  if (XR_SUCCEEDED (result))
    return TRUE;

  const char *resultString = xr_result_to_string (result);

  char buf[BUF_LEN] = {0};
  g_snprintf(buf, BUF_LEN, "[%s] ", resultString);

  va_list args;
  va_start (args, format);
  g_vsnprintf (buf, BUF_LEN - strlen (buf), "[%s]", args);
  va_end (args);

  g_warning("%s", buf);
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
_check_vk_extension (void)
{
  XrResult result;
  uint32_t instanceExtensionCount = 0;
  result = xrEnumerateInstanceExtensionProperties(
    NULL, 0, &instanceExtensionCount, NULL);

  if (!_check_xr_result (result,
      "Failed to enumerate number of instance extension properties"))
    return FALSE;

  XrExtensionProperties *instanceExtensionProperties =
    g_malloc (sizeof (XrExtensionProperties) * instanceExtensionCount);
  for (uint16_t i = 0; i < instanceExtensionCount; i++)
    instanceExtensionProperties[i] = (XrExtensionProperties){
      .type = XR_TYPE_EXTENSION_PROPERTIES,
    };

  result = xrEnumerateInstanceExtensionProperties (NULL, instanceExtensionCount,
                                                   &instanceExtensionCount,
                                                   instanceExtensionProperties);
  if (!_check_xr_result (result, "Failed to enumerate extension properties"))
    return FALSE;

  result =
    _is_extension_supported (XR_KHR_VULKAN_ENABLE_EXTENSION_NAME,
                             instanceExtensionProperties,
                             instanceExtensionCount);

  g_free (instanceExtensionProperties);

  if (!_check_xr_result (result,
      "Runtime does not support required instance extension %s\n",
       XR_KHR_VULKAN_ENABLE_EXTENSION_NAME))
    return FALSE;

  return TRUE;
}

static gboolean
_create_instance (GxrContext* self, char *app_name, uint32_t app_version)
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
      .applicationVersion = app_version,
      .engineName = "gxr",
      .engineVersion = GXR_VERSION_HEX,
      .apiVersion = XR_CURRENT_API_VERSION,
    },
  };

  strncpy(instanceCreateInfo.applicationInfo.applicationName,
          app_name, XR_MAX_APPLICATION_NAME_SIZE);

  XrResult result;
  result = xrCreateInstance (&instanceCreateInfo, &self->instance);
  if (!_check_xr_result (result, "Failed to create XR instance."))
    return FALSE;

  return TRUE;
}

static gboolean
_create_system (GxrContext* self)
{
  XrPath vrConfigName;
  XrResult result;
  result = xrStringToPath (self->instance, viewport_config_name, &vrConfigName);
  _check_xr_result (result, "failed to get viewport configuration name");

  g_debug ("Got vrconfig %lu\n", vrConfigName);

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
    .graphicsProperties = { 0 },
    .trackingProperties = { 0 },
  };

  result =
    xrGetSystemProperties (self->instance, self->system_id, &systemProperties);
  if (!_check_xr_result (result, "Failed to get System properties"))
    return FALSE;

  return TRUE;
}

static gboolean
_set_up_views (GxrContext* self)
{
  uint32_t viewConfigurationCount;
  XrResult result;
  result = xrEnumerateViewConfigurations (self->instance, self->system_id, 0,
                                          &viewConfigurationCount, NULL);
  if (!_check_xr_result (result, "Failed to get view configuration count"))
    return FALSE;

  XrViewConfigurationType *viewConfigurations =
    g_malloc (sizeof (XrViewConfigurationType) * viewConfigurationCount);
  result = xrEnumerateViewConfigurations(
    self->instance, self->system_id, viewConfigurationCount,
    &viewConfigurationCount, viewConfigurations);
  if (!_check_xr_result (result, "Failed to enumerate view configurations!"))
    return FALSE;

  self->view_config_type = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

  /* if struct (more specifically .type) is still 0 after searching, then
   we have not found the config. This way we don't need to set a bool
   found to TRUE. */
  XrViewConfigurationProperties requiredViewConfigProperties = { 0 };

  for (uint32_t i = 0; i < viewConfigurationCount; ++i)
    {
      XrViewConfigurationProperties properties = {
        .type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES,
      };

      result = xrGetViewConfigurationProperties ( self->instance,
                                                  self->system_id,
                                                  viewConfigurations[i],
                                                  &properties);
      if (!_check_xr_result (result,
          "Failed to get view configuration info %d!", i))
        return FALSE;

      if (viewConfigurations[i] == self->view_config_type &&
          properties.viewConfigurationType == self->view_config_type)
        requiredViewConfigProperties = properties;
    }

  g_free (viewConfigurations);

  if (requiredViewConfigProperties.type !=
      XR_TYPE_VIEW_CONFIGURATION_PROPERTIES)
    {
      g_print("Couldn't get required VR View Configuration %s from Runtime!\n",
                viewport_config_name);
      return FALSE;
    }

  result = xrEnumerateViewConfigurationViews (self->instance, self->system_id,
                                              self->view_config_type, 0,
                                              &self->view_count, NULL);

  self->views = g_malloc (sizeof(XrView) * self->view_count);
  for (uint32_t i = 0; i < self->view_count; i++)
    self->views[i].type = XR_TYPE_VIEW;

  if (!_check_xr_result (result,
      "Failed to get view configuration view count!"))
    return FALSE;

  self->configuration_views =
    malloc(sizeof(XrViewConfigurationView) * self->view_count);

  result = xrEnumerateViewConfigurationViews(
    self->instance, self->system_id, self->view_config_type, self->view_count,
    &self->view_count, self->configuration_views);
  if (!_check_xr_result
      (result, "Failed to enumerate view configuration views!"))
    return FALSE;

  return TRUE;
}

static gboolean
_check_graphics_api_support (GxrContext* self)
{
  XrGraphicsRequirementsVulkanKHR vk_reqs = {
    .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR,
  };
  PFN_xrGetVulkanGraphicsRequirementsKHR GetVulkanGraphicsRequirements = NULL;
  XrResult result =
    xrGetInstanceProcAddr(self->instance, "xrGetVulkanGraphicsRequirementsKHR",
      (PFN_xrVoidFunction*)(&GetVulkanGraphicsRequirements));
  if (!_check_xr_result (result,
      "Failed to retrieve OpenXR Vulkan function pointer!"))
    return FALSE;

  result =
    GetVulkanGraphicsRequirements (self->instance, self->system_id, &vk_reqs);
  if (!_check_xr_result (result,
      "Failed to get Vulkan graphics requirements!"))
    return FALSE;

  XrVersion desired_version = XR_MAKE_VERSION (1, 0, 0);
  if (desired_version > vk_reqs.maxApiVersionSupported ||
      desired_version < vk_reqs.minApiVersionSupported)
    {
      g_printerr ("Runtime does not support requested Vulkan version.\n");
      g_printerr ("desired_version %lu\n", desired_version);
      g_printerr ("minApiVersionSupported %lu\n", vk_reqs.minApiVersionSupported);
      g_printerr ("maxApiVersionSupported %lu\n", vk_reqs.maxApiVersionSupported);
      return FALSE;
    }
  return TRUE;
}

static gboolean
_init_runtime (GxrContext *self,
               char       *app_name,
               uint32_t    app_version)
{
  if (!_check_vk_extension())
    return FALSE;

  if (!_create_instance(self, app_name, app_version))
    return FALSE;

  if (!_create_system(self))
    return FALSE;

  if (!_set_up_views(self))
    return FALSE;

  if (!_check_graphics_api_support(self))
    return FALSE;

  return TRUE;
}

static gboolean
_create_session (GxrContext* self)
{
  XrSessionCreateInfo session_create_info = {
    .type = XR_TYPE_SESSION_CREATE_INFO,
    .next = &self->graphics_binding,
    .systemId = self->system_id,
  };

  XrResult result =
    xrCreateSession (self->instance, &session_create_info, &self->session);
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
_check_supported_spaces (GxrContext* self)
{
  uint32_t count;
  XrResult result = xrEnumerateReferenceSpaces (self->session, 0, &count, NULL);
  if (!_check_xr_result
      (result, "Getting number of reference spaces failed!"))
    return FALSE;

  XrReferenceSpaceType *spaces =
    g_malloc (sizeof (XrReferenceSpaceType) * count);
  result = xrEnumerateReferenceSpaces (self->session, count, &count, spaces);
  if (!_check_xr_result (result, "Enumerating reference spaces failed!"))
    return FALSE;

  if (!_is_space_supported (spaces, count, XR_REFERENCE_SPACE_TYPE_LOCAL)) {
    g_print ("XR_REFERENCE_SPACE_TYPE_LOCAL unsupported.\n");
    return FALSE;
  }

  if (_is_space_supported (spaces, count, XR_REFERENCE_SPACE_TYPE_STAGE))
    {
      self->play_space_type = XR_REFERENCE_SPACE_TYPE_STAGE;
      g_debug ("Stage space supported.");
    }
  else
    {
      self->play_space_type = XR_REFERENCE_SPACE_TYPE_LOCAL;
      g_debug ("Stage space not supported, fall back to local space!");
    }

  if (!_is_space_supported (spaces, count, XR_REFERENCE_SPACE_TYPE_VIEW)) {
    g_print ("XR_REFERENCE_SPACE_TYPE_VIEW unsupported.\n");
    return FALSE;
  }

  g_free (spaces);

   XrPosef space_pose = {
    .orientation = { .x = 0, .y = 0, .z = 0, .w = 1.0 },
    .position = { .x = 0, .y = 0, .z = 0 },
  };

  if (self->play_space_type == XR_REFERENCE_SPACE_TYPE_LOCAL)
    {
      space_pose.position.y = -1.6f;
      g_debug ("Using local space with %f y offset", space_pose.position.y);
    }

  XrReferenceSpaceCreateInfo info = {
    .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
    .referenceSpaceType = self->play_space_type,
    .poseInReferenceSpace = space_pose,
  };
  result = xrCreateReferenceSpace(self->session, &info, &self->play_space);
  if (!_check_xr_result (result, "Failed to create local space."))
    return FALSE;

  info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
  result = xrCreateReferenceSpace(self->session, &info, &self->view_space);
  if (!_check_xr_result (result, "Failed to create view space."))
    return FALSE;

  return TRUE;
}

static gboolean
_begin_session (GxrContext* self)
{
  XrSessionBeginInfo sessionBeginInfo = {
    .type = XR_TYPE_SESSION_BEGIN_INFO,
    .primaryViewConfigurationType = self->view_config_type,
  };
  XrResult result = xrBeginSession(self->session, &sessionBeginInfo);
  if (!_check_xr_result (result, "Failed to begin session!"))
    return FALSE;

  return TRUE;
}

static gboolean
_create_swapchains (GxrContext* self)
{
  XrResult result;
  uint32_t swapchainFormatCount;
  result =
    xrEnumerateSwapchainFormats (self->session, 0, &swapchainFormatCount, NULL);
  if (!_check_xr_result (result,
      "Failed to get number of supported swapchain formats"))
    return FALSE;

  int64_t *swapchainFormats =
    g_malloc (sizeof (int64_t) * swapchainFormatCount);
  result = xrEnumerateSwapchainFormats (self->session, swapchainFormatCount,
                                        &swapchainFormatCount, swapchainFormats);
  if (!_check_xr_result (result, "Failed to enumerate swapchain formats"))
    {
      g_free (swapchainFormats);
      return FALSE;
    }

  g_debug("Supported swapchain formats:");
  for (uint32_t i = 0; i < swapchainFormatCount; i++)
    g_debug("%s", vk_format_string ((VkFormat) swapchainFormats[i]));

  self->swapchain_format = VK_FORMAT_R8G8B8A8_SRGB;
  gboolean format_found = FALSE;
  for (uint32_t i = 0; i < swapchainFormatCount; i++)
    if (swapchainFormats[i] == self->swapchain_format)
      {
        format_found = TRUE;
        break;
      }

  if (!format_found)
    {
      g_warning ("Requested %s, but runtime doesn't support it.",
                 vk_format_string((VkFormat) self->swapchain_format));
      g_warning ("Using %s instead.",
                 vk_format_string((VkFormat) swapchainFormats[0]));
      self->swapchain_format = swapchainFormats[0];
    }


  self->swapchains = g_malloc (sizeof (XrSwapchain) * self->view_count);
  self->images =
    g_malloc (sizeof (XrSwapchainImageVulkanKHR*) * self->view_count);
  self->swapchain_length = g_malloc (sizeof (uint32_t) * self->view_count);

  for (uint32_t i = 0; i < self->view_count; i++)
    {
      /* make sure we don't clean up uninitialized pointer on failure */
      self->images[i] = NULL;

      XrSwapchainCreateInfo swapchainCreateInfo = {
        .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
        .usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
                      XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
        .createFlags = 0,
        // just use the first enumerated format
        .format = self->swapchain_format,
        .sampleCount = 1,
        .width = self->configuration_views[i].recommendedImageRectWidth,
        .height = self->configuration_views[i].recommendedImageRectHeight,
        .faceCount = 1,
        .arraySize = 1,
        .mipCount = 1,
      };

      g_debug ("Swapchain %d dimensions: %dx%d\n", i,
               self->configuration_views[i].recommendedImageRectWidth,
               self->configuration_views[i].recommendedImageRectHeight);

      result = xrCreateSwapchain (self->session, &swapchainCreateInfo,
                                  &self->swapchains[i]);
      if (!_check_xr_result (result, "Failed to create swapchain %d!", i))
        {
          g_free (swapchainFormats);
          return FALSE;
        }

      result = xrEnumerateSwapchainImages (self->swapchains[i], 0,
                                           &self->swapchain_length[i], NULL);
      if (!_check_xr_result (result, "Failed to enumerate swapchains"))
        {
          g_free (swapchainFormats);
          return FALSE;
        }


      self->images[i] =
        g_malloc (sizeof(XrSwapchainImageVulkanKHR) * self->swapchain_length[i]);

      result = xrEnumerateSwapchainImages(
        self->swapchains[i], self->swapchain_length[i],
        &self->swapchain_length[i],
        (XrSwapchainImageBaseHeader*)self->images[i]);
      if (!_check_xr_result (result, "Failed to enumerate swapchains"))
        {
          g_free (swapchainFormats);
          return FALSE;
        }
    }

  g_free (swapchainFormats);

  return TRUE;
}

static void
_create_projection_views (GxrContext* self)
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

static gboolean
_init_session (GxrContext *self)
{
  GulkanClient *gc = gxr_context_get_gulkan (self);
  GulkanDevice *gd = gulkan_client_get_device (gc);
  GulkanQueue *queue = gulkan_device_get_graphics_queue (gd);

  uint32_t family_index = gulkan_queue_get_family_index (queue);

  self->graphics_binding = (XrGraphicsBindingVulkanKHR){
    .type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR,
    .instance = gulkan_client_get_instance_handle (gc),
    .physicalDevice = gulkan_device_get_physical_handle (gd),
    .device = gulkan_device_get_handle (gd),
    .queueFamilyIndex = family_index,
    .queueIndex = 0,
  };

  if (!_create_session(self))
    return FALSE;

  if (!_check_supported_spaces(self))
    return FALSE;

  if (!_begin_session(self))
    return FALSE;

  if (!_create_swapchains(self))
    return FALSE;

  g_print("Created swapchains.\n");

  _create_projection_views(self);

  self->buffer_index = g_malloc (sizeof (uint32_t) * self->view_count);

  self->session_state = XR_SESSION_STATE_UNKNOWN;
  self->should_render = FALSE;
  self->have_valid_pose = FALSE;
  self->is_stopping = FALSE;

  self->projection_layer = (XrCompositionLayerProjection){
    .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
    .layerFlags = 0,
    .space = self->play_space,
    .viewCount = self->view_count,
    .views = self->projection_views,
  };

  return TRUE;
}

static void
_add_missing (GSList **target, GSList *source)
{
  for (GSList *l = source; l; l = l->next)
    {
      gchar *source_ext = l->data;
      if (!g_slist_find_custom (*target, source_ext, (GCompareFunc) g_strcmp0))
        *target = g_slist_append (*target, g_strdup (source_ext));
    }
}

static GxrContext *
_new (GSList     *instance_ext_list,
      GSList     *device_ext_list,
      char       *app_name,
      uint32_t    app_version)
{
  GxrContext *self = (GxrContext*) g_object_new (GXR_TYPE_CONTEXT, 0);
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

  GSList *runtime_instance_ext_list = NULL;
  gxr_context_get_instance_extensions (self, &runtime_instance_ext_list);

  /* vk instance required for checking required device exts */
  GulkanClient *gc_tmp =
    gulkan_client_new_from_extensions (runtime_instance_ext_list, NULL);

  if (!gc_tmp)
    {
      g_object_unref (self);
      g_slist_free_full (runtime_instance_ext_list, g_free);
      g_printerr ("Could not init gulkan client from instance exts.\n");
      return NULL;
    }

  GSList *runtime_device_ext_list = NULL;

  gxr_context_get_device_extensions (self, gc_tmp, &runtime_device_ext_list);

  g_object_unref (gc_tmp);

  _add_missing (&runtime_instance_ext_list, instance_ext_list);
  _add_missing (&runtime_device_ext_list, device_ext_list);

  self->gc =
    gulkan_client_new_from_extensions (runtime_instance_ext_list, runtime_device_ext_list);

  g_slist_free_full (runtime_instance_ext_list, g_free);
  g_slist_free_full (runtime_device_ext_list, g_free);

  if (!self->gc)
    {
      g_object_unref (self);
      g_printerr ("Could not init gulkan client.\n");
      return NULL;
    }

  if (!_init_session (self))
    {
      g_object_unref (self);
      g_printerr ("Could not init VR session.\n");
      return NULL;
    }

  return self;
}

GxrContext *gxr_context_new (char      *app_name,
                             uint32_t   app_version)
{
  return gxr_context_new_from_vulkan_extensions (NULL, NULL, app_name, app_version);
}

GxrContext *gxr_context_new_from_vulkan_extensions (GSList *instance_ext_list,
                                                    GSList *device_ext_list,
                                                    char       *app_name,
                                                    uint32_t    app_version)
{
  return _new (instance_ext_list, device_ext_list,
               app_name, app_version);
}

static void
gxr_context_init (GxrContext *self)
{
  self->gc = NULL;
  self->device_manager = gxr_device_manager_new ();
  self->view_count = 0;
  self->views = NULL;
  self->manifest = NULL;
  self->predicted_display_time = 0;
  self->predicted_display_period = 0;
  self->framebuffer = NULL;
  self->images = NULL;
}

static void
_cleanup (GxrContext *self)
{
  if (!self->swapchains)
    {
      for (uint32_t i = 0; i < self->view_count; i++) {
        xrDestroySwapchain (self->swapchains[i]);
      }
    }

  if (self->swapchains)
    g_free (self->swapchains);
  if (self->play_space)
    xrDestroySpace (self->play_space);
  if (self->session)
    xrDestroySession (self->session);
  if (self->instance)
    xrDestroyInstance (self->instance);

  g_free (self->configuration_views);

  g_free (self->views);
  g_free (self->projection_views);

  if (self->framebuffer)
    {
      for (uint32_t view = 0; view < self->view_count; view++)
        {
          for (uint32_t i = 0; i < self->swapchain_length[view]; i++)
            {
              if (GULKAN_IS_FRAME_BUFFER (self->framebuffer[view][i]))
                g_object_unref (self->framebuffer[view][i]);
              else
                g_printerr ("Failed to release framebuffer %d for view %d\n",
                            i, view);
            }
          g_free (self->framebuffer[view]);
        }
      g_free (self->framebuffer);
    }

  g_free (self->swapchain_length);

  if (self->images)
    {
      for (uint32_t i = 0; i < self->view_count; i++)
        g_free (self->images[i]);
      g_free (self->images);
    }

  g_free (self->buffer_index);
}

static void
gxr_context_finalize (GObject *gobject)
{
  GxrContext *self = GXR_CONTEXT (gobject);

  _cleanup (self);
  g_clear_object (&self->manifest);

  GulkanClient *gulkan = gxr_context_get_gulkan (GXR_CONTEXT (gobject));

  if (gulkan)
    g_object_unref (gulkan);

  if (self->device_manager != NULL)
    g_clear_object (&self->device_manager);

  /* child classes MUST destroy gulkan after this destructor finishes */

  G_OBJECT_CLASS (gxr_context_parent_class)->finalize (gobject);
}

GulkanClient*
gxr_context_get_gulkan (GxrContext *self)
{
  return self->gc;
}

static gboolean
_space_location_valid (XrSpaceLocation *sl)
{
  return (sl->locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)    != 0 &&
         (sl->locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0;
}

static void
_get_model_matrix_from_pose (XrPosef           *pose,
                             graphene_matrix_t *mat)
{
  graphene_quaternion_t q;
  graphene_quaternion_init (&q,
                            pose->orientation.x, pose->orientation.y,
                            pose->orientation.z, pose->orientation.w);

  graphene_matrix_t rotation;
  graphene_matrix_init_identity (&rotation);
  graphene_matrix_rotate_quaternion (&rotation, &q);

  graphene_point3d_t position = {
    pose->position.x,
    pose->position.y,
    pose->position.z
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
    .next = NULL
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
#define RAD_TO_DEG(x) ( (x) * 360.0f / ( 2.0f * PI ) )

void
gxr_context_get_frustum_angles (GxrContext *self, GxrEye eye,
                                float *left, float *right,
                                float *top, float *bottom)
{
  *left = RAD_TO_DEG (self->views[eye].fov.angleLeft);
  *right = RAD_TO_DEG (self->views[eye].fov.angleRight);
  *top = RAD_TO_DEG (self->views[eye].fov.angleUp);
  *bottom = RAD_TO_DEG (self->views[eye].fov.angleDown);

  // g_debug ("Fov angles L %f R %f T %f B %f", *left, *right, *top, *bottom);
}

// TODO: remove
void
gxr_context_get_render_dimensions (GxrContext *self,
                                   VkExtent2D *extent)
{
  gxr_context_get_swapchain_dimensions (self, 0, extent);
}

void
gxr_context_poll_event (GxrContext *self)
{
  XrEventDataBuffer runtimeEvent = {
    .type = XR_TYPE_EVENT_DATA_BUFFER,
    .next = NULL,
  };

  XrResult pollResult = xrPollEvent (self->instance, &runtimeEvent);

  while (pollResult == XR_SUCCESS)
    {
      switch (runtimeEvent.type)
      {
        case XR_TYPE_EVENT_DATA_EVENTS_LOST:
        {
          /* We didnt poll enough */
          g_printerr ("Event: Events lost\n");
        } break;
        case XR_TYPE_EVENT_DATA_MAIN_SESSION_VISIBILITY_CHANGED_EXTX:
        {
          g_debug ("Event: STUB: Session visibility changed\n");
        } break;
        case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT:
        {
          g_debug ("Event: STUB: perf settings\n");
        } break;
        case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR:
        {
          g_debug ("Event: STUB: visibility mask changed\n");
        } break;
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
        {
          XrEventDataSessionStateChanged* event =
            (XrEventDataSessionStateChanged*)&runtimeEvent;

          self->session_state = event->state;
          g_debug ("EVENT: session state changed to %d\n", event->state);
          if (event->state >= XR_SESSION_STATE_STOPPING)
            {
              self->is_stopping = TRUE;

              GxrQuitEvent *quit_event = g_malloc (sizeof (GxrQuitEvent));
              quit_event->reason = GXR_QUIT_SHUTDOWN;
              g_debug ("Event: sending VR_QUIT_SHUTDOWN signal\n");
              gxr_context_emit_quit (self, quit_event);

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
          gxr_context_emit_quit (self, event);
          break;
        }
        case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
        {
          g_print ("Event: STUB: interaction profile changed\n");

          XrInteractionProfileState state = {
            .type = XR_TYPE_INTERACTION_PROFILE_STATE
          };

          char *hand_str[2] = { "/user/hand/left", "/user/hand/right" };
          XrPath hand_paths[2];
          xrStringToPath(self->instance, hand_str[0], &hand_paths[0]);
          xrStringToPath(self->instance, hand_str[1], &hand_paths[1]);
          for (int i = 0; i < NUM_CONTROLLERS; i++)
            {


              XrResult res =
                xrGetCurrentInteractionProfile (self->session,
                                                hand_paths[i],
                                                &state);
              if (!_check_xr_result (res,
                "Failed to get interaction profile for %s", hand_str[i]))
                continue;

              XrPath prof = state.interactionProfile;

              if (prof == XR_NULL_PATH)
                {
                  // perhaps no controller is present
                  g_debug ("Event: Interaction profile on %s: [none]",
                           hand_str[i]);
                  continue;
                }

              uint32_t strl;
              char profile_str[XR_MAX_PATH_LENGTH];
              res = xrPathToString (self->instance, prof, XR_MAX_PATH_LENGTH,
                                    &strl, profile_str);
              if (!_check_xr_result (res,
                "Failed to get interaction profile path str for %s",
                hand_str[i]))
                continue;

              g_debug ("Event: Interaction profile on %s: %s",
                       hand_str[i], profile_str);
            }

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

void
gxr_context_emit_keyboard_press (GxrContext *self, gpointer event)
{
  g_signal_emit (self, context_signals[KEYBOARD_PRESS_EVENT], 0, event);
}

void
gxr_context_emit_keyboard_close (GxrContext *self)
{
  g_signal_emit (self, context_signals[KEYBOARD_CLOSE_EVENT], 0);
}

void
gxr_context_emit_quit (GxrContext *self, gpointer event)
{
  g_signal_emit (self, context_signals[QUIT_EVENT], 0, event);
}


void
gxr_context_emit_device_update (GxrContext *self, gpointer event)
{
  g_signal_emit (self, context_signals[DEVICE_UPDATE_EVENT], 0, event);
}

void
gxr_context_emit_bindings_update (GxrContext *self)
{
  g_signal_emit (self, context_signals[BINDINGS_UPDATE], 0);
}

void
gxr_context_emit_binding_loaded (GxrContext *self)
{
  g_signal_emit (self, context_signals[BINDING_LOADED], 0);
}

void
gxr_context_emit_actionset_update (GxrContext *self)
{
  g_signal_emit (self, context_signals[ACTIONSET_UPDATE], 0);
}

gboolean
gxr_context_init_framebuffers (GxrContext           *self,
                               VkExtent2D            extent,
                               VkSampleCountFlagBits sample_count,
                               GulkanRenderPass    **render_pass)
{
  VkFormat format = gxr_context_get_swapchain_format (self);

  GulkanClient *gc = gxr_context_get_gulkan (self);
  GulkanDevice *device = gulkan_client_get_device (gc);

  self->framebuffer_extent = extent;
  self->framebuffer_sample_count = sample_count;

  *render_pass =
    gulkan_render_pass_new (device, sample_count, format,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, TRUE);
  if (!*render_pass)
    {
      g_printerr ("Could not init render pass.\n");
      return FALSE;
    }

  self->framebuffer = g_malloc (sizeof (GulkanFrameBuffer*) * self->view_count);
  for (uint32_t view = 0; view < self->view_count; view++)
    {
      g_debug ("Creating %d framebuffers for view %d\n",
               self->swapchain_length[view], view);

      self->framebuffer[view] =
        g_malloc (sizeof (GulkanFrameBuffer*) * self->swapchain_length[view]);
      for (uint32_t i = 0; i < self->swapchain_length[view]; i++)
        {

          self->framebuffer[view][i] =
            gulkan_frame_buffer_new_from_image_with_depth (device, *render_pass,
                                                           self->images[view][i].image,
                                                           extent, sample_count,
                                                           format);

          if (!GULKAN_IS_FRAME_BUFFER (self->framebuffer[view][i]))
            {
              g_printerr ("Could not initialize frambuffer.");
              return FALSE;
            }
        }
  }

  return TRUE;
}

/*
 * Dummy values to make the pipeline initialize,
 * TODO: Add GLTF models for OpenXR backend.
 */
uint32_t
gxr_context_get_model_vertex_stride (GxrContext *self)
{
  (void) self;
  return 1;
}

uint32_t
gxr_context_get_model_normal_offset (GxrContext *self)
{
  (void) self;
  return 1;
}

uint32_t
gxr_context_get_model_uv_offset (GxrContext *self)
{
  (void) self;
  return 2;
}

static void
_get_projection_matrix_from_fov (const XrFovf       fov,
                                 const float        near_z,
                                 const float        far_z,
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

void
gxr_context_get_projection (GxrContext *self,
                            GxrEye eye,
                            float near,
                            float far,
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
_get_view_matrix_from_pose (XrPosef           *pose,
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
gxr_context_get_view (GxrContext *self,
                      GxrEye eye,
                      graphene_matrix_t *mat)
{
  if (self->views == NULL)
    {
      g_warning ("get_view needs to be called between begin and end frame.\n");
      graphene_matrix_init_identity (mat);
      return;
    }
  _get_view_matrix_from_pose (&self->views[eye].pose, mat);
}

static gboolean
_begin_frame (GxrContext* self)
{
  XrResult result;

  XrFrameState frame_state = {
    .type = XR_TYPE_FRAME_STATE,
    .next = NULL
  };
  XrFrameWaitInfo frameWaitInfo = {
    .type = XR_TYPE_FRAME_WAIT_INFO,
    .next = NULL
  };
  result = xrWaitFrame (self->session, &frameWaitInfo, &frame_state);
  if (!_check_xr_result
      (result, "xrWaitFrame() was not successful, exiting..."))
    return FALSE;

  self->should_render = frame_state.shouldRender == XR_TRUE;

  self->predicted_display_time = frame_state.predictedDisplayTime;
  self->predicted_display_period = frame_state.predictedDisplayPeriod;

  if (self->session_state == XR_SESSION_STATE_EXITING ||
      self->session_state == XR_SESSION_STATE_LOSS_PENDING ||
      self->session_state == XR_SESSION_STATE_STOPPING)
    return FALSE;

  // --- Create projection matrices and view matrices for each eye
  XrViewLocateInfo viewLocateInfo = {
    .type = XR_TYPE_VIEW_LOCATE_INFO,
    .displayTime = frame_state.predictedDisplayTime,
    .space = self->play_space,
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

  self->have_valid_pose =
   (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) != 0 &&
   (viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) != 0;

  return TRUE;
}

static gboolean
_aquire_swapchain (GxrContext* self,
                   uint32_t i,
                   uint32_t *buffer_index)
{
  XrResult result;

  XrSwapchainImageAcquireInfo swapchainImageAcquireInfo = {
    .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO,
  };

  result = xrAcquireSwapchainImage (self->swapchains[i],
                                    &swapchainImageAcquireInfo, buffer_index);
  if (!_check_xr_result (result, "failed to acquire swapchain image!"))
    return FALSE;

  XrSwapchainImageWaitInfo swapchainImageWaitInfo = {
    .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
    .timeout = INT64_MAX,
  };
  result = xrWaitSwapchainImage (self->swapchains[i], &swapchainImageWaitInfo);
  if (!_check_xr_result (result, "failed to wait for swapchain image!"))
    return FALSE;

  self->projection_views[i].pose = self->views[i].pose;
  self->projection_views[i].fov = self->views[i].fov;
  self->projection_views[i].subImage.imageArrayIndex = 0;

  return TRUE;
}

gboolean
gxr_context_begin_frame (GxrContext *self)
{
  if (!_begin_frame (self)) {
    g_printerr ("Could not begin XR frame.\n");
    return FALSE;
  }

  for (uint32_t i = 0; i < 2; i++) {
    if (!_aquire_swapchain (self, i, &self->buffer_index[i]))
      {
        g_printerr ("Could not aquire xr swapchain\n");
        return FALSE;
      }
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
  XrResult result;

  const XrCompositionLayerBaseHeader* const projection_layers[1] = {
    (const XrCompositionLayerBaseHeader* const) &self->projection_layer
  };
  XrFrameEndInfo frame_end_info = {
    .type = XR_TYPE_FRAME_END_INFO,
    .displayTime = self->predicted_display_time,
    .layerCount = self->have_valid_pose ? 1 : 0,
    .layers = self->have_valid_pose ? projection_layers : NULL,
    .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
  };
  result = xrEndFrame (self->session, &frame_end_info);
  if (!_check_xr_result (result, "failed to end frame!"))
    return FALSE;

  return TRUE;
}


gboolean
gxr_context_end_frame (GxrContext *self)
{
  for (uint32_t i = 0; i < 2; i++)
  if (!gxr_context_release_swapchain (self, i)) {
      g_printerr ("Could not release xr swapchain\n");
      return FALSE;
  }

  if (!_end_frame (self)) {
    g_printerr ("Could not end xr frame\n");
  }

  return TRUE;
}


gboolean
gxr_context_load_action_manifest (GxrContext *self,
                                  const char *cache_name,
                                  const char *resource_path,
                                  const char *manifest_name)
{
  (void) self;
  (void) cache_name;

  GError *error = NULL;
  GString *actions_res_path = g_string_new ("");
  g_string_printf (actions_res_path, "%s/%s", resource_path, manifest_name);

  /* stream can not be reset/reused, has to be recreated */
  GInputStream *actions_res_input_stream =
  g_resources_open_stream (actions_res_path->str,
                           G_RESOURCE_LOOKUP_FLAGS_NONE,
                           &error);
  if (error)
    {
      g_print ("Unable to open stream: %s\n", error->message);
      g_error_free (error);
      return FALSE;
    }

  self->manifest = gxr_manifest_new ();
  if (!gxr_manifest_load_actions (self->manifest, actions_res_input_stream))
  {
    g_printerr ("Failed to load action manifest\n");
    return FALSE;
  }

  if (!gxr_manifest_load_bindings (self->manifest, resource_path))
  {
    g_printerr ("Failed to load action binding manifests\n");
    return FALSE;
  }

  g_object_unref (actions_res_input_stream);
  g_string_free (actions_res_path, TRUE);
  g_debug ("Loaded action manifest");

  return TRUE;
}

void
gxr_context_acknowledge_quit (GxrContext *self)
{
  /* TODO: Implement in OpenXR */
  (void) self;
}

gboolean
gxr_context_is_tracked_device_connected (GxrContext *self, uint32_t i)
{
  /* TODO: Implement in OpenXR */
  (void) self;
  (void) i;
  return FALSE;
}

gboolean
gxr_context_device_is_controller (GxrContext *self, uint32_t i)
{
  /* TODO: Implement in OpenXR */
  (void) self;
  (void) i;
  return FALSE;
}

gchar*
gxr_context_get_device_model_name (GxrContext *self, uint32_t i)
{
  /* TODO: Implement in OpenXR */
  (void) self;
  (void) i;
  return NULL;
}

gboolean
gxr_context_load_model (GxrContext         *self,
                        GulkanVertexBuffer *vbo,
                        GulkanTexture     **texture,
                        VkSampler          *sampler,
                        const char         *model_name)
{
  /* TODO: Implement in OpenXR */
  (void) self;
  (void) vbo;
  (void) texture;
  (void) sampler;
  (void) model_name;
  return FALSE;
}

void
gxr_context_request_quit (GxrContext *self)
{
  /* _poll_event will send quit event once session is in STOPPING state */
  xrRequestExitSession (self->session);
}

GSList *
gxr_context_get_model_list (GxrContext *self)
{
  /* TODO: Implement in OpenXR */
  (void) self;
  return NULL;
}


gboolean
gxr_context_get_instance_extensions (GxrContext *self, GSList **out_list)
{
  (void) self;
  (void) out_list;
  g_print ("Stub: get instance extensions\n");
  return TRUE;
}

gboolean
gxr_context_get_device_extensions (GxrContext   *self,
                                   GulkanClient *gc,
                                   GSList      **out_list)
{
  (void) self;
  (void) gc;
  (void) out_list;
  g_print ("Stub: get device extensions\n");
  return TRUE;
}

GxrDeviceManager *
gxr_context_get_device_manager (GxrContext *self)
{
  return self->device_manager;
}

uint32_t
gxr_context_get_view_count (GxrContext *self)
{
  return self->view_count;
}

GulkanFrameBuffer *
gxr_context_get_acquired_framebuffer (GxrContext *self, uint32_t view)
{
  GulkanFrameBuffer *fb =
    GULKAN_FRAME_BUFFER (self->framebuffer[view][self->buffer_index[view]]);
  return fb;
}

XrSessionState
gxr_context_get_session_state (GxrContext *self)
{
  return self->session_state;
}

GxrManifest *
gxr_context_get_manifest (GxrContext *self)
{
  return self->manifest;
}

XrTime
gxr_context_get_predicted_display_time (GxrContext *self)
{
  return self->predicted_display_time;
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

VkFormat
gxr_context_get_swapchain_format (GxrContext *self)
{
  return (VkFormat) self->swapchain_format;
}

gboolean
gxr_context_release_swapchain (GxrContext * self, uint32_t eye)
{
  XrSwapchainImageReleaseInfo swapchainImageReleaseInfo = {
    .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
  };
  XrResult result =
    xrReleaseSwapchainImage (self->swapchains[eye], &swapchainImageReleaseInfo);
  if (!_check_xr_result (result, "failed to release swapchain image!"))
    return FALSE;

  return TRUE;
}

void
gxr_context_get_swapchain_dimensions (GxrContext *self,
                                      uint32_t       i,
                                      VkExtent2D    *extent)
{
  extent->width = self->configuration_views[i].recommendedImageRectWidth;
  extent->height = self->configuration_views[i].recommendedImageRectHeight;
}

XrSwapchainImageVulkanKHR**
gxr_context_get_images (GxrContext *self)
{
  return self->images;
}

void
gxr_context_get_position (GxrContext   *self,
                          uint32_t         i,
                          graphene_vec4_t *v)
{
  graphene_vec4_init (v,
                      self->views[i].pose.position.x,
                      self->views[i].pose.position.y,
                      self->views[i].pose.position.z,
                      1.0f);
}
