/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 *
 * Based on openvr hellovr example.
 */

#include "openvr-vulkan-renderer.h"
#include "openvr-global.h"

#include "openvr_capi_global.h"

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#include <gmodule.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "openvr-overlay.h"

G_DEFINE_TYPE (OpenVRVulkanRenderer, openvr_vulkan_renderer, G_TYPE_OBJECT)

static void
openvr_vulkan_renderer_finalize (GObject *gobject);


VulkanCommandBuffer_t*
_get_command_buffer2 (OpenVRVulkanRenderer *self);

bool
_init_device2 (OpenVRVulkanRenderer *self);

static void
openvr_vulkan_renderer_class_init (OpenVRVulkanRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_vulkan_renderer_finalize;
}

static void
openvr_vulkan_renderer_init (OpenVRVulkanRenderer *self)
{
  self->instance = openvr_vulkan_instance_new ();
  self->device = openvr_vulkan_device_new ();
  self->command_pool = VK_NULL_HANDLE;
  self->cmd_buffers = g_queue_new ();
}

OpenVRVulkanRenderer *
openvr_vulkan_renderer_new (void)
{
  return (OpenVRVulkanRenderer*) g_object_new (OPENVR_TYPE_VULKAN_RENDERER, 0);
}

void
_cleanup_command_buffer_queue2 (gpointer item, OpenVRVulkanRenderer *self)
{
  VulkanCommandBuffer_t *b = (VulkanCommandBuffer_t*) item;
  vkFreeCommandBuffers (self->device->device,
                        self->command_pool, 1, &b->cmd_buffer);
  vkDestroyFence (self->device->device, b->fence, NULL);
  // g_object_unref (item);
}

static void
openvr_vulkan_renderer_finalize (GObject *gobject)
{
  OpenVRVulkanRenderer *self = OPENVR_VULKAN_RENDERER (gobject);

  /* Idle the device to make sure no work is outstanding */
  if (self->device->device != VK_NULL_HANDLE)
    vkDeviceWaitIdle (self->device->device);

  if (self->device->device != VK_NULL_HANDLE)
  {
    g_queue_foreach (self->cmd_buffers,
                     (GFunc) _cleanup_command_buffer_queue2, self);

    vkDestroyCommandPool (self->device->device, self->command_pool, NULL);

    g_object_unref (self->device);
    g_object_unref (self->instance);
  }

  G_OBJECT_CLASS (openvr_vulkan_renderer_parent_class)->finalize (gobject);
}

bool
_init_command_pool2 (OpenVRVulkanRenderer *self)
{
  VkCommandPoolCreateInfo command_pool_info =
    {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = self->device->queue_family_index,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    };

  VkResult result = vkCreateCommandPool (self->device->device,
                                         &command_pool_info,
                                         NULL, &self->command_pool);
  if (result != VK_SUCCESS)
    {
      g_printerr ("vkCreateCommandPool returned error %d.", result);
      return false;
    }
  return true;
}

void
_begin_command_buffer2 (OpenVRVulkanRenderer *self)
{
  /* Command buffer used during resource loading */
  self->current_cmd_buffer = _get_command_buffer2 (self);
  VkCommandBufferBeginInfo command_buffer_begin_info =
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  };
  vkBeginCommandBuffer (self->current_cmd_buffer->cmd_buffer,
                        &command_buffer_begin_info);
}

void
_submit_command_buffer2 (OpenVRVulkanRenderer *self)
{
  /* Submit the command buffer used during loading */
  vkEndCommandBuffer (self->current_cmd_buffer->cmd_buffer);

  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &self->current_cmd_buffer->cmd_buffer
  };

  vkQueueSubmit (self->device->queue, 1, &submit_info, self->current_cmd_buffer->fence);
  g_queue_push_head (self->cmd_buffers, (gpointer) self->current_cmd_buffer);

  /* Wait for the GPU before proceeding */
  vkQueueWaitIdle (self->device->queue);
}

bool
openvr_vulkan_renderer_load_raw (OpenVRVulkanRenderer *self,
                                 OpenVRVulkanTexture  *texture,
                                 guchar               *pixels,
                                 guint                 width,
                                 guint                 height,
                                 gsize                 size,
                                 VkFormat              format)
{
  _begin_command_buffer2 (self);


  if (!openvr_vulkan_texture_from_pixels (texture,
                                          self->device,
                                          self->current_cmd_buffer->cmd_buffer,
                                          pixels, width, height, size, format))
    return false;

  _submit_command_buffer2 (self);

  return true;
}

bool
openvr_vulkan_renderer_load_dmabuf2 (OpenVRVulkanRenderer *self,
                                     OpenVRVulkanTexture  *texture,
                                     int                   fd,
                                     guint                 width,
                                     guint                 height,
                                     VkFormat              format)
{
  _begin_command_buffer2 (self);


  if (!openvr_vulkan_texture_from_dmabuf2 (texture,
                                          self->device,
                                          self->current_cmd_buffer->cmd_buffer,
                                          fd, width, height, format))
    return false;

  _submit_command_buffer2 (self);

  return true;
}

bool
openvr_vulkan_renderer_load_pixbuf (OpenVRVulkanRenderer *self,
                                    OpenVRVulkanTexture  *texture,
                                    GdkPixbuf *pixbuf)
{
  guint width = (guint) gdk_pixbuf_get_width (pixbuf);
  guint height = (guint) gdk_pixbuf_get_height (pixbuf);
  guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);
  gsize size = gdk_pixbuf_get_byte_length (pixbuf);

  return openvr_vulkan_renderer_load_raw (self, texture, pixels,
                                          width, height, size,
                                          VK_FORMAT_R8G8B8A8_UNORM);
}

bool
openvr_vulkan_renderer_load_cairo_surface (OpenVRVulkanRenderer *self,
                                           OpenVRVulkanTexture  *texture,
                                           cairo_surface_t *surface)
{
  guint width = cairo_image_surface_get_width (surface);
  guint height = cairo_image_surface_get_height (surface);

  VkFormat format;
  cairo_format_t cr_format = cairo_image_surface_get_format (surface);
  switch (cr_format)
    {
    case CAIRO_FORMAT_ARGB32:
      format = VK_FORMAT_B8G8R8A8_UNORM;
      break;
    case CAIRO_FORMAT_RGB24:
    case CAIRO_FORMAT_A8:
    case CAIRO_FORMAT_A1:
    case CAIRO_FORMAT_RGB16_565:
    case CAIRO_FORMAT_RGB30:
      g_printerr ("Unsupported Cairo format\n");
      return FALSE;
    case CAIRO_FORMAT_INVALID:
      g_printerr ("Invalid Cairo format\n");
      return FALSE;
    default:
      g_printerr ("Unknown Cairo format\n");
      return FALSE;
    }

  guchar *pixels = cairo_image_surface_get_data (surface);

  int stride = cairo_image_surface_get_stride (surface);

  gsize size = stride * height;

  return openvr_vulkan_renderer_load_raw (self, texture, pixels,
                                          width, height, size, format);
}

bool
_query_surface_present_modes (VkPhysicalDevice device,
                              VkSurfaceKHR surface,
                              VkPresentModeKHR *present_modes)
{
  uint32_t num_present_modes;
  vkGetPhysicalDeviceSurfacePresentModesKHR (device, surface,
                                            &num_present_modes, NULL);

  if (num_present_modes != 0)
    {
      present_modes = g_malloc (sizeof(VkPresentModeKHR) * num_present_modes);
      vkGetPhysicalDeviceSurfacePresentModesKHR (device, surface,
                                                &num_present_modes,
                                                 present_modes);
    }
  else
    {
      g_printerr ("Could enumerate present modes.\n");
      return false;
    }

  return true;
}

bool
_query_surface_formats (VkPhysicalDevice device,
                        VkSurfaceKHR surface,
                        VkSurfaceFormatKHR *formats)
{
  uint32_t num_formats;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &num_formats, NULL);

  if (num_formats != 0)
    {
      formats = g_malloc (sizeof(VkSurfaceFormatKHR) * num_formats);
      vkGetPhysicalDeviceSurfaceFormatsKHR (device, surface,
                                           &num_formats, formats);
    }
  else
    {
      g_printerr ("Could enumerate surface formats.\n");
      return false;
    }

  return true;
}

VkImageView
_create_image_view (VkDevice device, VkImage image, VkFormat format)
{
  VkImageViewCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image = image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = format,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
  };

  VkImageView image_view;
  if (vkCreateImageView (device, &info, NULL, &image_view) != VK_SUCCESS)
    {
      g_printerr ("Cound not create texture image view.\n");
    }

  return image_view;
}

void
_create_render_pass (VkDevice device, VkFormat format)
{
  VkRenderPassCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &(VkAttachmentDescription) {
      .format = format,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    },
    .subpassCount = 1,
    .pSubpasses = &(VkSubpassDescription) {
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &(VkAttachmentReference) {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
      }
    },
    .dependencyCount = 1,
    .pDependencies = &(VkSubpassDependency) {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                       VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    }
  };

  VkRenderPass render_pass;
  if (vkCreateRenderPass (device, &info, NULL, &render_pass) != VK_SUCCESS)
    {
      g_printerr ("Failed to create render pass.");
    }

  vkDestroyRenderPass (device, render_pass, NULL);
}

void
_create_descriptor_set_layout (VkDevice device)
{
  VkDescriptorSetLayoutCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 2,
    .pBindings = (VkDescriptorSetLayoutBinding[]) {
      {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      },
      {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      }
    }
  };

  VkDescriptorSetLayout descriptor_set_layout;

  if (vkCreateDescriptorSetLayout (device, &info,
                                   NULL, &descriptor_set_layout) != VK_SUCCESS)
    {
      g_printerr ("Failed to create descriptor set layout.");
    }

  vkDestroyDescriptorSetLayout (device, descriptor_set_layout, NULL);
}

bool
openvr_vulkan_renderer_init_swapchain (VkDevice device,
                                       VkPhysicalDevice physical_device,
                                       VkSurfaceKHR surface)
{
  VkSurfaceCapabilitiesKHR surface_caps;

  VkSurfaceFormatKHR surface_format = {
    .format = VK_FORMAT_B8G8R8A8_UNORM,
    .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
  };

  VkSurfaceFormatKHR *formats = NULL;
  _query_surface_formats (physical_device, surface, formats);

  VkPresentModeKHR *present_modes = NULL;
  _query_surface_present_modes (physical_device, surface, present_modes);

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR (physical_device,
                                             surface, &surface_caps);

  VkFormat swapchain_image_format = surface_format.format;

  VkSwapchainCreateInfoKHR swapchain_info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = surface,
    .minImageCount = surface_caps.minImageCount,
    .imageFormat = swapchain_image_format,
    .imageColorSpace = surface_format.colorSpace,
    .imageExtent = surface_caps.currentExtent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .clipped = VK_TRUE
  };

  VkSwapchainKHR swap_chain;

  VkResult res =
    vkCreateSwapchainKHR (device, &swapchain_info, NULL, &swap_chain);

  if (res != VK_SUCCESS)
    {
      g_printerr ("Unable to create swap chain.\n");
      return false;
    }
  else
    {
      g_print ("Created swapchain successfully.\n");
    }

  uint32_t image_count;
  res = vkGetSwapchainImagesKHR (device, swap_chain, &image_count, NULL);

  g_print ("The swapchain has %d images\n", image_count);

  VkImage *swap_chain_images = g_malloc (sizeof(VkImage) * image_count);
  res = vkGetSwapchainImagesKHR (device, swap_chain,
                                &image_count, swap_chain_images);

  if (res != VK_SUCCESS)
    {
      g_printerr ("Unable to get swapchain images.\n");
      return false;
    }

  VkImageView *swapchain_image_views = g_malloc (sizeof(VkImage) * image_count);
  for (int i = 0; i < image_count; i++)
    swapchain_image_views[i] = _create_image_view (device,
                                                   swap_chain_images[i],
                                                   swapchain_image_format);

  _create_render_pass (device, swapchain_image_format);

  _create_descriptor_set_layout (device);

  for (int i = 0; i < image_count; i++)
    vkDestroyImageView (device, swapchain_image_views[i], NULL);

  vkDestroySwapchainKHR(device, swap_chain, NULL);

  return true;
}

bool
openvr_vulkan_renderer_init_vulkan (OpenVRVulkanRenderer *self,
                                    VkSurfaceKHR surface,
                                    bool enable_validation)
{

  const gchar *instance_extensions[] =
  {
    VK_KHR_SURFACE_EXTENSION_NAME,
    "VK_KHR_xcb_surface" // VK_KHR_XCB_SURFACE_EXTENSION_NAME
  };

  GSList *instance_ext_list = NULL;
  for (int i = 0; i < G_N_ELEMENTS (instance_extensions); i++)
    {
      instance_ext_list = g_slist_append (instance_ext_list,
                                          g_strdup (instance_extensions[i]));
    }

  if (!openvr_vulkan_instance_create (self->instance,
                                      enable_validation,
                                      instance_ext_list))
    {
      g_printerr ("Failed to create instance.\n");
      return false;
    }

  const gchar *device_extensions[] =
  {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,
    VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME
  };

  GSList *device_ext_list = NULL;
  for (int i = 0; i < G_N_ELEMENTS (device_extensions); i++)
    {
      device_ext_list = g_slist_append (device_ext_list,
                                        g_strdup (device_extensions[i]));
    }

  if (!openvr_vulkan_device_create (self->device, self->instance,
                                    VK_NULL_HANDLE, device_ext_list))
    {
      g_printerr ("Failed to create device.\n");
      return false;
    }

  if(!_init_command_pool2 (self))
    {
      g_printerr ("Failed to create command pool.\n");
      return false;
    }

  return true;
}

void
openvr_vulkan_renderer_submit_frame (OpenVRVulkanRenderer *self,
                                     OpenVROverlay        *overlay,
                                     OpenVRVulkanTexture  *texture)
{
  /* Submit to SteamVR */
  struct VRVulkanTextureData_t texture_data =
    {
      .m_nImage = (uint64_t) texture->image,
      .m_pDevice = (struct VkDevice_T*) self->device->device,
      .m_pPhysicalDevice = (struct VkPhysicalDevice_T*)
        self->device->physical_device,
      .m_pInstance = (struct VkInstance_T*) self->instance->instance,
      .m_pQueue = (struct VkQueue_T*) self->device->queue,
      .m_nQueueFamilyIndex = self->device->queue_family_index,
      .m_nWidth = texture->width,
      .m_nHeight = texture->height,
      .m_nFormat = VK_FORMAT_B8G8R8A8_UNORM,
      .m_nSampleCount = 1
    };

  struct Texture_t vr_texture =
    {
      .handle = &texture_data,
      .eType = ETextureType_TextureType_Vulkan,
      .eColorSpace = EColorSpace_ColorSpace_Auto
    };

  overlay->functions->SetOverlayTexture (overlay->overlay_handle, &vr_texture);
}

/*
 * Get an available command buffer or create a new one if one available.
 * Associate a fence with the command buffer.
 */
VulkanCommandBuffer_t*
_get_command_buffer2 (OpenVRVulkanRenderer *self)
{
  VulkanCommandBuffer_t *command_buffer = g_new (VulkanCommandBuffer_t, 1);

  if (g_queue_get_length (self->cmd_buffers) > 0)
  {
    /*
     * If the fence associated with the command buffer has finished,
     * reset it and return it
     */
    VulkanCommandBuffer_t *tail =
      (VulkanCommandBuffer_t*) g_queue_peek_tail(self->cmd_buffers);

    if (vkGetFenceStatus (self->device->device, tail->fence) == VK_SUCCESS)
    {
      command_buffer->cmd_buffer = tail->cmd_buffer;
      command_buffer->fence = tail->fence;

      vkResetCommandBuffer (command_buffer->cmd_buffer,
                            VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
      vkResetFences (self->device->device, 1, &command_buffer->fence);
      gpointer last = g_queue_pop_tail (self->cmd_buffers);
      g_free (last);

      return command_buffer;
    }
  }

  /* Create a new command buffer and associated fence */
  VkCommandBufferAllocateInfo command_buffer_info =
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandBufferCount = 1,
    .commandPool = self->command_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
  };
  vkAllocateCommandBuffers (self->device->device, &command_buffer_info,
                            &command_buffer->cmd_buffer);

  VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
  };
  vkCreateFence (self->device->device, &fence_info,
                 NULL, &command_buffer->fence);
  return command_buffer;
}
