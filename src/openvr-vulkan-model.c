/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-model.h"
#include "openvr-context.h"
#include <gulkan-descriptor-set.h>

G_DEFINE_TYPE (OpenVRVulkanModel, openvr_vulkan_model, G_TYPE_OBJECT)

static void
openvr_vulkan_model_finalize (GObject *gobject);

static void
openvr_vulkan_model_class_init (OpenVRVulkanModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_vulkan_model_finalize;
}

static void
openvr_vulkan_model_init (OpenVRVulkanModel *self)
{
  memset (self->ubos, 0, sizeof (self->ubos));
  self->ubos[0] = gulkan_uniform_buffer_new ();
  self->ubos[1] = gulkan_uniform_buffer_new ();

  memset (self->descriptor_sets, 0, sizeof (self->descriptor_sets));

  self->content = NULL;
}

OpenVRVulkanModel *
openvr_vulkan_model_new (void)
{
  return (OpenVRVulkanModel*) g_object_new (OPENVR_TYPE_VULKAN_MODEL, 0);
}

static void
openvr_vulkan_model_finalize (GObject *gobject)
{
  OpenVRVulkanModel *self = OPENVR_VULKAN_MODEL (gobject);

  vkDestroyDescriptorPool (self->device->device,
                           self->descriptor_pool, NULL);
  g_object_unref (self->content);

  for (uint32_t i = 0; i < 2; i++)
    g_object_unref (self->ubos[i]);
}

gboolean
openvr_vulkan_model_init_descriptors (OpenVRVulkanModel     *self,
                                      VkDescriptorSetLayout *layout)
{
  uint32_t set_count = 2;

  VkDescriptorPoolSize pool_sizes[] = {
    {
      .descriptorCount = set_count,
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    },
    {
      .descriptorCount = set_count,
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    }
  };

  if (!GULKAN_INIT_DECRIPTOR_POOL (self->device, pool_sizes,
                                   set_count, &self->descriptor_pool))
     return FALSE;

  for (uint32_t eye = 0; eye < set_count; eye++)
    if (!gulkan_allocate_descritpor_set (self->device, self->descriptor_pool,
                                         layout, 1,
                                         &self->descriptor_sets[eye]))
      return FALSE;

  return TRUE;
}


gboolean
openvr_vulkan_model_initialize (OpenVRVulkanModel        *self,
                                OpenVRVulkanModelContent *content,
                                GulkanDevice             *device,
                                VkDescriptorSetLayout    *layout)
{
  self->device = device;

  if (!openvr_vulkan_model_init_descriptors (self, layout))
    return FALSE;

  self->content = content;
  g_object_ref (self->content);

  for (uint32_t i = 0; i < 2; i++)
    {
      if (!gulkan_uniform_buffer_allocate_and_map (self->ubos[i], device,
                                                   sizeof (float) * 16))
        {
          g_printerr ("Could not create Uniform buffer.\n");
          return FALSE;
        }

      VkWriteDescriptorSet *write_descriptors = (VkWriteDescriptorSet []) {
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = self->descriptor_sets[i],
          .dstBinding = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .pBufferInfo = &(VkDescriptorBufferInfo) {
            .buffer = self->ubos[i]->buffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE
          },
          .pTexelBufferView = NULL
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = self->descriptor_sets[i],
          .dstBinding = 1,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .pImageInfo = &(VkDescriptorImageInfo) {
            .sampler = self->content->sampler,
            .imageView = self->content->texture->image_view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
          },
          .pTexelBufferView = NULL
        }
      };

      vkUpdateDescriptorSets (device->device, 2, write_descriptors, 0, NULL);
    }

  return TRUE;
}

void
openvr_vulkan_model_draw (OpenVRVulkanModel *self,
                          EVREye             eye,
                          VkCommandBuffer    cmd_buffer,
                          VkPipelineLayout   pipeline_layout,
                          graphene_matrix_t *mvp)
{
  graphene_matrix_to_float (mvp, (float*) self->ubos[eye]->data);

  vkCmdBindDescriptorSets (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           pipeline_layout, 0, 1, &self->descriptor_sets[eye],
                           0, NULL);

  gulkan_vertex_buffer_draw_indexed (self->content->vbo, cmd_buffer);
}

