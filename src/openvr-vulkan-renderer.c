/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#include <gmodule.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <graphene.h>

#include "openvr-vulkan-renderer.h"

#define MAX_FRAMES_IN_FLIGHT 2

typedef struct Vertex {
  float position[2];
  float uv[2];
} Vertex;

const Vertex vertices[4] = {
    {{-1.f, -1.f}, {1.f, 0.f}},
    {{ 1.f, -1.f}, {0.f, 0.f}},
    {{ 1.f,  1.f}, {0.f, 1.f}},
    {{-1.f,  1.f}, {1.f, 1.f}}
};

const uint16_t indices[6] = {0, 1, 2, 2, 3, 0};

typedef struct Transformation {
  float mvp[16];
} Transformation;

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
  self->current_frame = 0;
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

  VkDevice device = self->device->device;

  /* Idle the device to make sure no work is outstanding */
  if (device != VK_NULL_HANDLE)
    vkDeviceWaitIdle (device);

  /* Rendering */
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
      vkDestroySemaphore (device, self->render_finished_semaphores[i], NULL);
      vkDestroySemaphore (device, self->image_available_semaphores[i], NULL);
      vkDestroyFence (device, self->in_flight_fences[i], NULL);
    }

  vkDestroyDescriptorPool (device, self->descriptor_pool, NULL);

  vkDestroySampler (device, self->sampler, NULL);

  vkDestroyBuffer (device, self->vertex_buffer, NULL);
  vkFreeMemory (device, self->vertex_buffer_memory, NULL);

  vkDestroyBuffer (device, self->index_buffer, NULL);
  vkFreeMemory (device, self->index_buffer_memory, NULL);

  for (size_t i = 0; i < self->swapchain_image_count; i++)
    {
      vkDestroyBuffer (device, self->uniform_buffers[i], NULL);
      vkFreeMemory (device, self->uniform_buffers_memory[i], NULL);
    }

  for (int i = 0; i < self->swapchain_image_count; i++)
    vkDestroyFramebuffer(device, self->framebuffers[i], NULL);

  vkDestroyPipeline (device, self->graphics_pipeline, NULL);
  vkDestroyPipelineLayout (device, self->pipeline_layout, NULL);

  vkDestroyDescriptorSetLayout (device, self->descriptor_set_layout, NULL);

  vkDestroyRenderPass (device, self->render_pass, NULL);

  for (int i = 0; i < self->swapchain_image_count; i++)
    vkDestroyImageView (device, self->swapchain_image_views[i], NULL);

  vkDestroySwapchainKHR(device, self->swap_chain, NULL);

  /* Instance and devices */
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
openvr_vulkan_renderer_load_dmabuf (OpenVRVulkanRenderer *self,
                                    OpenVRVulkanTexture  *texture,
                                    int                   fd,
                                    guint                 width,
                                    guint                 height,
                                    VkFormat              format)
{
  _begin_command_buffer2 (self);


  if (!openvr_vulkan_texture_from_dmabuf (texture, self->device,
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

bool
_create_render_pass (VkDevice device,
                     VkFormat format,
                     VkRenderPass *render_pass)
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

  if (vkCreateRenderPass (device, &info, NULL, render_pass) != VK_SUCCESS)
    {
      g_printerr ("Failed to create render pass.");
      return false;
    }

  return true;
}

bool
_create_descriptor_set_layout (VkDevice device,
                               VkDescriptorSetLayout *descriptor_set_layout)
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

  if (vkCreateDescriptorSetLayout (device, &info,
                                   NULL, descriptor_set_layout) != VK_SUCCESS)
    {
      g_printerr ("Failed to create descriptor set layout.");
      return false;
    }

  return true;
}

bool
_create_graphics_pipeline (VkDevice device,
                           VkShaderModule vert_shader,
                           VkShaderModule frag_shader,
                           VkExtent2D extent,
                           VkDescriptorSetLayout descriptor_set_layout,
                           VkRenderPass render_pass,
                           VkPipeline *graphics_pipeline,
                           VkPipelineLayout *pipeline_layout)
{
  VkPipelineLayoutCreateInfo layout_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = &descriptor_set_layout
  };

  VkResult res;
  res = vkCreatePipelineLayout (device, &layout_info, NULL, pipeline_layout);

  if (res != VK_SUCCESS)
    {
      g_printerr ("Failed to create pipeline layout.\n");
      return false;
    }

  VkGraphicsPipelineCreateInfo pipeline_info = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = 2,
    .pStages = (VkPipelineShaderStageCreateInfo[]) {
      {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vert_shader,
        .pName = "main"
      },
      {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = frag_shader,
        .pName = "main"
      }
    },
    .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &(VkVertexInputBindingDescription) {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
      },
      .vertexAttributeDescriptionCount = 2,
      .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]) {
        {
          .location = 0,
          .binding = 0,
          .format = VK_FORMAT_R32G32_SFLOAT,
          .offset = offsetof (Vertex, position)
        },
        {
          .location = 1,
          .binding = 0,
          .format = VK_FORMAT_R32G32_SFLOAT,
          .offset = offsetof (Vertex, uv)
        }
      },
    },
    .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    },
    .pViewportState = &(VkPipelineViewportStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &(VkViewport) {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float) extent.width,
        .height = (float) extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
      },
      .scissorCount = 1,
      .pScissors = &(VkRect2D) {
        .offset = {0, 0},
        .extent = extent
      }
    },
    .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0f
    },
    .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    },
    .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &(VkPipelineColorBlendAttachmentState) {
        .colorWriteMask =
          VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
      },
    },
    .layout = *pipeline_layout,
    .renderPass = render_pass
  };

  res = vkCreateGraphicsPipelines (device, VK_NULL_HANDLE, 1,
                                  &pipeline_info, NULL, graphics_pipeline);

  if (res != VK_SUCCESS)
    {
      g_printerr ("Failed to create graphics pipeline.\n");
      return false;
    }

  return true;
}

bool
_find_surface_format (VkPhysicalDevice device,
                      VkSurfaceKHR surface,
                      VkSurfaceFormatKHR *format)
{
  uint32_t num_formats;
  VkSurfaceFormatKHR *formats = NULL;
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

  for (int i = 0; i < num_formats; i++)
    if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
        formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
        format->format = formats[i].format;
        format->colorSpace = formats[i].colorSpace;
        return true;
      }

  g_printerr ("Requested format not supported.\n");
  return false;
}

bool
_find_surface_present_mode (VkPhysicalDevice device,
                            VkSurfaceKHR surface,
                            VkPresentModeKHR *present_mode)
{
  uint32_t num_present_modes;
  VkPresentModeKHR *present_modes;
  vkGetPhysicalDeviceSurfacePresentModesKHR (device, surface,
                                            &num_present_modes, NULL);

  if (num_present_modes != 0)
    {
      present_modes = g_malloc (sizeof (VkPresentModeKHR) * num_present_modes);
      vkGetPhysicalDeviceSurfacePresentModesKHR (device, surface,
                                                &num_present_modes,
                                                 present_modes);
    }
  else
    {
      g_printerr ("Could enumerate present modes.\n");
      return false;
    }

  for (int i = 0; i < num_present_modes; i++)
    if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
      {
        *present_mode = present_modes[i];
        return true;
      }

  g_printerr ("Requested present mode not supported.\n");
  return false;
}

bool
_init_swapchain (VkDevice device,
                 VkPhysicalDevice physical_device,
                 VkSurfaceKHR surface,
                 VkSwapchainKHR *swapchain,
                 VkImageView **image_views,
                 VkFormat *image_format,
                 uint32_t *image_count,
                 VkExtent2D *extent)
{
  VkSurfaceFormatKHR surface_format = {};
  if (!_find_surface_format(physical_device, surface, &surface_format))
    return false;

  VkPresentModeKHR present_mode;
  if (!_find_surface_present_mode (physical_device, surface, &present_mode))
    return false;

  VkSurfaceCapabilitiesKHR surface_caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR (physical_device,
                                             surface, &surface_caps);

  *image_format = surface_format.format;
  *extent = surface_caps.currentExtent;

  VkSwapchainCreateInfoKHR info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = surface,
    .minImageCount = surface_caps.minImageCount,
    .imageFormat = *image_format,
    .imageColorSpace = surface_format.colorSpace,
    .imageExtent = *extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = present_mode,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .clipped = VK_TRUE
  };

  VkResult res = vkCreateSwapchainKHR (device, &info, NULL, swapchain);
  if (res != VK_SUCCESS)
    {
      g_printerr ("Unable to create swap chain.\n");
      return false;
    }

  res = vkGetSwapchainImagesKHR (device, *swapchain, image_count, NULL);

  g_print ("The swapchain has %d images\n", *image_count);

  VkImage *images = g_malloc (sizeof(VkImage) * *image_count);
  res = vkGetSwapchainImagesKHR (device, *swapchain, image_count, images);

  if (res != VK_SUCCESS)
    {
      g_printerr ("Unable to get swapchain images.\n");
      return false;
    }

  *image_views = g_malloc (sizeof(VkImage) * *image_count);
  for (int i = 0; i < *image_count; i++)
    (*image_views)[i] = _create_image_view (device, images[i], *image_format);
  return true;
}

bool
_load_resource (const gchar* path, GBytes **res)
{
  GError *error = NULL;

  *res = g_resources_lookup_data (path, G_RESOURCE_FLAGS_NONE, &error);

  if (error != NULL)
    {
      g_printerr ("Unable to read file: %s\n", error->message);
      g_error_free (error);
      return false;
    }

  return true;
}

bool
_create_shader_module (VkDevice device,
                       GBytes *bytes,
                       VkShaderModule *module)
{
  gsize size = 0;
  const uint32_t *buffer = g_bytes_get_data (bytes, &size);

  VkShaderModuleCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = size,
    .pCode = buffer,
  };

  if (vkCreateShaderModule (device, &info, NULL, module) != VK_SUCCESS)
    {
      g_printerr ("Failed to create shader module.\n");
      return false;
    }

  return true;
}

bool
_init_framebuffers (VkDevice device,
                    VkRenderPass render_pass,
                    VkExtent2D swapchain_extent,
                    uint32_t swapchain_image_count,
                    VkImageView *swapchain_image_views,
                    VkFramebuffer **framebuffers)
{
  *framebuffers = g_malloc (sizeof(VkFramebuffer) * swapchain_image_count);

  for (size_t i = 0; i < swapchain_image_count; i++)
    {
      VkFramebufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = 1,
        .pAttachments = &swapchain_image_views[i],
        .width = swapchain_extent.width,
        .height = swapchain_extent.height,
        .layers = 1
        };

      VkResult ret =
        vkCreateFramebuffer(device, &info, NULL, &((*framebuffers)[i]));
      if (ret != VK_SUCCESS)
        {
          g_printerr ("Failed to create framebuffer.\n");
          return false;
        }
    }
  return true;
}

bool
_create_buffer (VkDevice device,
                OpenVRVulkanDevice *vr_device,
                VkDeviceSize size,
                VkBufferUsageFlags usage,
                VkMemoryPropertyFlags properties,
                VkBuffer *buffer,
                VkDeviceMemory *memory)
{
  VkBufferCreateInfo buffer_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE
  };

  if (vkCreateBuffer (device, &buffer_info, NULL, buffer) != VK_SUCCESS)
    {
      g_printerr ("Failed to create buffer.\n");
      return false;
    }

  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements (device, *buffer, &requirements);

  VkMemoryAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = requirements.size,
    //.memoryTypeIndex = findMemoryType (requirements.memoryTypeBits, properties)
  };

  if (!openvr_vulkan_device_memory_type_from_properties (
        vr_device,
        requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
       &alloc_info.memoryTypeIndex))
  {
    g_printerr ("Failed to find matching memoryTypeIndex for buffer\n");
    return false;
  }

  if (vkAllocateMemory (device, &alloc_info, NULL, memory) != VK_SUCCESS)
    {
      g_printerr ("Failed to allocate memory.\n");
      return false;
    }

  vkBindBufferMemory (device, *buffer, *memory, 0);

  return true;
}

void
_copy_buffer (VkCommandBuffer cmd_buffer,
              VkBuffer src_buffer,
              VkBuffer dst_buffer,
              VkDeviceSize size)
{
  VkBufferCopy copy_region = {
    .size = size
  };

  vkCmdCopyBuffer (cmd_buffer, src_buffer, dst_buffer, 1, &copy_region);
}

bool
_init_vertex_buffer (OpenVRVulkanRenderer *self)
{
  VkDeviceSize buffer_size = sizeof (vertices[0]) * 4;

  VkDevice device = self->device->device;

  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  if (!_create_buffer (device, self->device, buffer_size,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      &staging_buffer, &staging_buffer_memory))
    return false;

  void* data;
  vkMapMemory (device, staging_buffer_memory, 0, buffer_size, 0, &data);
  memcpy (data, vertices, (size_t) buffer_size);
  vkUnmapMemory (device, staging_buffer_memory);

  if (!_create_buffer (device, self->device, buffer_size,
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      &self->vertex_buffer, &self->vertex_buffer_memory))
    return false;

  _begin_command_buffer2 (self);

  _copy_buffer (self->current_cmd_buffer->cmd_buffer,
                staging_buffer, self->vertex_buffer, buffer_size);

  _submit_command_buffer2 (self);

  vkDestroyBuffer (device, staging_buffer, NULL);
  vkFreeMemory (device, staging_buffer_memory, NULL);

  return true;
}

bool
_init_index_buffer (OpenVRVulkanRenderer *self)
{
  VkDeviceSize buffer_size = sizeof (indices[0]) * 6;

  VkDevice device = self->device->device;

  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  if (!_create_buffer (device, self->device, buffer_size,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      &staging_buffer, &staging_buffer_memory))
    return false;

  void* data;
  vkMapMemory (device, staging_buffer_memory, 0, buffer_size, 0, &data);
  memcpy(data, indices, (size_t) buffer_size);
  vkUnmapMemory (device, staging_buffer_memory);

  if (!_create_buffer (device, self->device, buffer_size,
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      &self->index_buffer, &self->index_buffer_memory))
    return false;

  _begin_command_buffer2 (self);

  _copy_buffer (self->current_cmd_buffer->cmd_buffer,
                staging_buffer, self->index_buffer, buffer_size);

  _submit_command_buffer2 (self);

  vkDestroyBuffer (device, staging_buffer, NULL);
  vkFreeMemory (device, staging_buffer_memory, NULL);

  return true;
}

bool
_init_uniform_buffers (OpenVRVulkanRenderer *self)
{
  VkDeviceSize buffer_size = sizeof(Transformation);

  uint32_t count = self->swapchain_image_count;

  self->uniform_buffers = g_malloc (sizeof (VkBuffer) * count);
  self->uniform_buffers_memory = g_malloc (sizeof (VkDeviceMemory) * count);

  for (size_t i = 0; i < count; i++)
    if (!_create_buffer (self->device->device, self->device,
                         buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        &self->uniform_buffers[i],
                        &self->uniform_buffers_memory[i]))
      return false;

  return true;
}

bool
_init_descriptor_pool (VkDevice device,
                       uint32_t image_count,
                       VkDescriptorPool *descriptor_pool)
{
  VkDescriptorPoolCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = image_count,
    .poolSizeCount = 2,
    .pPoolSizes = (VkDescriptorPoolSize[]) {
      {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = image_count
      },
      {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = image_count
      }
    }
  };

  VkResult res = vkCreateDescriptorPool (device, &info, NULL, descriptor_pool);
  if (res != VK_SUCCESS)
    {
      g_printerr ("Failed to create descriptor pool.\n");
      return false;
    }

  return true;
}

bool
_init_descriptor_sets (VkDevice device,
                       VkDescriptorSetLayout descriptor_set_layout,
                       VkDescriptorPool descriptor_pool,
                       uint32_t count,
                       VkBuffer *uniform_buffers,
                       VkSampler texture_sampler,
                       VkImageView texture_image_view,
                       VkDescriptorSet **descriptor_sets)
{
  VkDescriptorSetLayout* layouts = g_malloc (sizeof (VkBuffer) * count);
  for (int i = 0; i < count; i++)
    layouts[i] = descriptor_set_layout;

  VkDescriptorSetAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptor_pool,
    .descriptorSetCount = count,
    .pSetLayouts = layouts
  };

  *descriptor_sets = g_malloc (sizeof (VkDescriptorSet) * count);

  VkResult res =
    vkAllocateDescriptorSets (device, &alloc_info, &((*descriptor_sets)[0]));

  if (res != VK_SUCCESS)
    {
      g_printerr ("Failed to allocate descriptor sets.\n");
      return false;
    }

  for (size_t i = 0; i < count; i++)
    {
      VkWriteDescriptorSet *descriptor_writes = (VkWriteDescriptorSet [])
      {
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = (*descriptor_sets)[i],
          .dstBinding = 0,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .pBufferInfo = &(VkDescriptorBufferInfo) {
            .buffer = uniform_buffers[i],
            .offset = 0,
            .range = sizeof (Transformation),
          }
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = (*descriptor_sets)[i],
          .dstBinding = 1,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .pImageInfo = &(VkDescriptorImageInfo) {
            .sampler = texture_sampler,
            .imageView = texture_image_view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
          }
        }
      };

      vkUpdateDescriptorSets (device, 2, descriptor_writes, 0, NULL);
    }

  return true;
}

bool
_init_texture_sampler (VkDevice device, VkSampler *sampler)
{
  VkSamplerCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .anisotropyEnable = VK_TRUE,
    .maxAnisotropy = 16,
    .compareEnable = VK_FALSE,
    .compareOp = VK_COMPARE_OP_ALWAYS,
    .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    .unnormalizedCoordinates = VK_FALSE
    };

  if (vkCreateSampler(device, &info, NULL, sampler) != VK_SUCCESS)
    {
      g_printerr ("Failed to create sampler.\n");
      return false;
    }

  return true;
}

bool
_init_draw_cmd_buffers (VkDevice device,
                        VkCommandPool command_pool,
                        VkRenderPass render_pass,
                        VkFramebuffer *framebuffers,
                        VkExtent2D swapchain_extent,
                        VkPipeline pipeline,
                        VkBuffer vertex_buffer,
                        VkBuffer index_buffer,
                        VkPipelineLayout pipeline_layout,
                        VkDescriptorSet *descriptor_sets,
                        uint32_t count,
                        VkCommandBuffer **cmd_buffers)
{
  *cmd_buffers = g_malloc (sizeof (VkCommandBuffer) * count);

  VkCommandBufferAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = command_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = count
  };

  VkResult ret = vkAllocateCommandBuffers (device, &alloc_info, *cmd_buffers);
  if (ret != VK_SUCCESS)
    {
      g_printerr ("Failed to allocate command buffers.\n");
      return false;
    }

  for (size_t i = 0; i < count; i++)
    {
      VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
      };

      VkCommandBuffer cmd_buffer = (*cmd_buffers)[i];

      if (vkBeginCommandBuffer (cmd_buffer, &begin_info) != VK_SUCCESS)
        {
          g_printerr ("Failed to begin command buffer.\n");
          return false;
        }

      VkClearValue clear_color = {
        .color = {
          .float32 = {0.0f, 0.0f, 0.0f, 1.0f}
        }
      };

      VkRenderPassBeginInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass,
        .framebuffer = framebuffers[i],
        .renderArea = {
          .offset = {0, 0},
          .extent = swapchain_extent
        },
        .clearValueCount = 1,
        .pClearValues = &clear_color
      };

      vkCmdBeginRenderPass (cmd_buffer, &render_pass_info,
                            VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindPipeline (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                         pipeline);

      VkBuffer vertex_buffers[] = { vertex_buffer };
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers (cmd_buffer, 0, 1, vertex_buffers, offsets);

      vkCmdBindIndexBuffer (cmd_buffer, index_buffer, 0,
                            VK_INDEX_TYPE_UINT16);

      vkCmdBindDescriptorSets (cmd_buffer,
                               VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout,
                               0, 1, &descriptor_sets[i], 0, NULL);

      vkCmdDrawIndexed (cmd_buffer, G_N_ELEMENTS (indices), 1, 0, 0, 0);

      vkCmdEndRenderPass (cmd_buffer);

      if (vkEndCommandBuffer (cmd_buffer) != VK_SUCCESS)
        {
          g_printerr ("Failed to end command buffer.\n");
          return false;
        }
    }

  return true;
}

bool
_init_synchronization (VkDevice device,
                       VkSemaphore **image_available_semaphores,
                       VkSemaphore **render_finished_semaphores,
                       VkFence **in_flight_fences)
{
  *image_available_semaphores =
    g_malloc (sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
  *render_finished_semaphores =
    g_malloc (sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
  *in_flight_fences = g_malloc (sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphore_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
  };

  VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT
  };

  VkResult ret;
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
      ret = vkCreateSemaphore (device, &semaphore_info,
                               NULL, &((*image_available_semaphores)[i]));
      if (ret != VK_SUCCESS)
        {
          g_printerr ("Failed to create semaphore.\n");
          return false;
        }

      ret = vkCreateSemaphore (device, &semaphore_info,
                               NULL, &((*render_finished_semaphores)[i]));
      if (ret != VK_SUCCESS)
        {
          g_printerr ("Failed to create semaphore.\n");
          return false;
        }

      ret = vkCreateFence (device, &fence_info,
                           NULL, &((*in_flight_fences)[i]));
      if (ret != VK_SUCCESS)
        {
          g_printerr ("Failed to create fence.\n");
          return false;
        }
    }

  return true;
}

bool
openvr_vulkan_renderer_init_rendering (OpenVRVulkanRenderer *self,
                                       VkSurfaceKHR surface,
                                       OpenVRVulkanTexture *texture)
{
  VkDevice device = self->device->device;

  g_print ("Device surface support: %d\n",
           openvr_vulkan_device_queue_supports_surface (self->device,
                                                        surface));

  if (!_init_swapchain (device,
                        self->device->physical_device,
                        surface,
                       &self->swap_chain,
                       &self->swapchain_image_views,
                       &self->swapchain_image_format,
                       &self->swapchain_image_count,
                       &self->swapchain_extent))
      return false;

  _create_render_pass (device,
                       self->swapchain_image_format,
                       &self->render_pass);

  _create_descriptor_set_layout (device, &self->descriptor_set_layout);

  GBytes *fragment_shader;
  GBytes *vertex_shader;

  if (!_load_resource ("/shaders/texture.frag.spv", &fragment_shader))
    return false;
  if (!_load_resource ("/shaders/texture.vert.spv", &vertex_shader))
    return false;

  VkShaderModule fragment_shader_module;
  if (!_create_shader_module (device, fragment_shader, &fragment_shader_module))
    return false;

  VkShaderModule vertex_shader_module;
  if (!_create_shader_module (device, vertex_shader, &vertex_shader_module))
    return false;

  g_bytes_unref (fragment_shader);
  g_bytes_unref (vertex_shader);

  if (!_create_graphics_pipeline (device,
                                  vertex_shader_module,
                                  fragment_shader_module,
                                  self->swapchain_extent,
                                  self->descriptor_set_layout,
                                  self->render_pass,
                                 &self->graphics_pipeline,
                                 &self->pipeline_layout))
    return false;

  vkDestroyShaderModule (device, fragment_shader_module, NULL);
  vkDestroyShaderModule (device, vertex_shader_module, NULL);

  if (!_init_framebuffers (device,
                           self->render_pass,
                           self->swapchain_extent,
                           self->swapchain_image_count,
                           self->swapchain_image_views,
                          &self->framebuffers))
    return false;

  if (!_init_vertex_buffer (self))
    return false;

  if (!_init_index_buffer (self))
    return false;

  if (!_init_uniform_buffers (self))
    return false;

  if (!_init_descriptor_pool (device,
                              self->swapchain_image_count,
                             &self->descriptor_pool))
    return false;

  if (!_init_texture_sampler (device, &self->sampler))
    return false;

  if (!_init_descriptor_sets (device,
                             self->descriptor_set_layout,
                             self->descriptor_pool,
                             self->swapchain_image_count,
                             self->uniform_buffers,
                             self->sampler,
                             texture->image_view,
                            &self->descriptor_sets))
    return false;

  if (!_init_draw_cmd_buffers (device,
                               self->command_pool,
                               self->render_pass,
                               self->framebuffers,
                               self->swapchain_extent,
                               self->graphics_pipeline,
                               self->vertex_buffer,
                               self->index_buffer,
                               self->pipeline_layout,
                               self->descriptor_sets,
                               self->swapchain_image_count,
                              &self->draw_cmd_buffers))
    return false;

  if (!_init_synchronization (device,
                             &self->image_available_semaphores,
                             &self->render_finished_semaphores,
                             &self->in_flight_fences))
    return false;

  return true;
}

bool
_update_uniform_buffer (VkDevice device,
                        VkDeviceMemory uniform_buffer_memory)
{
  graphene_vec3_t eye;
  graphene_vec3_init (&eye, 0.0f, 0.0f, -1.0f);

  graphene_vec3_t center;
  graphene_vec3_init (&center, 0.0f, 0.0f, 0.0f);

  graphene_matrix_t view;
  graphene_matrix_init_look_at (&view, &eye, &center, graphene_vec3_y_axis());

  graphene_matrix_t ortho;
  graphene_matrix_init_ortho (&ortho, -1.f, 1.f, -1.f, 1.f, 1.f, -1.f);

  graphene_matrix_t mvp;
  graphene_matrix_multiply (&view, &ortho, &mvp);

  Transformation ubo = {};
  graphene_matrix_to_float (&mvp, ubo.mvp);

  void* data;
  vkMapMemory (device, uniform_buffer_memory, 0, sizeof(ubo), 0, &data);
  memcpy (data, &ubo, sizeof(ubo));
  vkUnmapMemory (device, uniform_buffer_memory);

  return true;
}

bool
openvr_vulkan_renderer_draw (OpenVRVulkanRenderer *self)
{
  VkDevice device = self->device->device;

  vkWaitForFences (device, 1,
                   &self->in_flight_fences[self->current_frame],
                   VK_TRUE, UINT64_MAX);

  uint32_t image_index;
  VkResult result =
    vkAcquireNextImageKHR (device, self->swap_chain, UINT64_MAX,
                           self->image_available_semaphores[self->current_frame],
                           VK_NULL_HANDLE, &image_index);

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
      g_printerr ("Failed to acquire swap chain image.\n");
      return false;
    }

  _update_uniform_buffer (device, self->uniform_buffers_memory[image_index]);

  VkPipelineStageFlags wait_stages[] = {
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
  };

  VkSemaphore wait_semaphores[] = {self->image_available_semaphores[self->current_frame]};
  VkSemaphore signal_semaphores[] = {self->render_finished_semaphores[self->current_frame]};

  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = wait_semaphores,
    .pWaitDstStageMask = wait_stages,
    .commandBufferCount = 1,
    .pCommandBuffers = &self->draw_cmd_buffers[image_index],
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = signal_semaphores
  };

  vkResetFences (device, 1, &self->in_flight_fences[self->current_frame]);

  if (vkQueueSubmit (self->device->queue, 1, &submit_info,
                     self->in_flight_fences[self->current_frame]) != VK_SUCCESS)
    {
      g_printerr ("Failed to submit draw command buffer.\n");
      return false;
    }

  VkSwapchainKHR swapchains[] = { self->swap_chain };

  VkPresentInfoKHR present_info = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = signal_semaphores,
    .swapchainCount = 1,
    .pSwapchains = swapchains,
    .pImageIndices = &image_index,
  };

  result = vkQueuePresentKHR (self->device->present_queue, &present_info);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) // || framebuffer_resized
    {
      // framebuffer_resized = false;
      // _recreate_swapchain ();
      g_printerr ("VK_ERROR_OUT_OF_DATE_KHR or VK_SUBOPTIMAL_KHR.\n");
      return false;
    }
  else if (result != VK_SUCCESS)
    {
      g_printerr ("Failed to present swapchain image.\n");
      return false;
    }

  self->current_frame = (self->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

  return true;
}

bool
openvr_vulkan_renderer_init_vulkan (OpenVRVulkanRenderer *self,
                                    GSList *instance_extensions,
                                    GSList *device_extensions,
                                    bool enable_validation)
{

  if (!openvr_vulkan_instance_create (self->instance,
                                      enable_validation,
                                      instance_extensions))
    {
      g_printerr ("Failed to create instance.\n");
      return false;
    }

  if (!openvr_vulkan_device_create (self->device, self->instance,
                                    VK_NULL_HANDLE, device_extensions))
    {
      g_printerr ("Failed to create device.\n");
      return false;
    }

  if (!_init_command_pool2 (self))
    {
      g_printerr ("Failed to create command pool.\n");
      return false;
    }

  return true;
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
