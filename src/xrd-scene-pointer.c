/*
 * Xrd GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-pointer.h"
#include "gulkan-geometry.h"
#include <gulkan-descriptor-set.h>

G_DEFINE_TYPE (XrdScenePointer, xrd_scene_pointer, G_TYPE_OBJECT)

static void
xrd_scene_pointer_finalize (GObject *gobject);

static void
xrd_scene_pointer_class_init (XrdScenePointerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_pointer_finalize;
}

static void
xrd_scene_pointer_init (XrdScenePointer *self)
{
  self->vertex_buffer = gulkan_vertex_buffer_new ();
  self->descriptor_pool = VK_NULL_HANDLE;
  for (uint32_t eye = 0; eye < 2; eye++)
    self->uniform_buffers[eye] = gulkan_uniform_buffer_new ();

}

XrdScenePointer *
xrd_scene_pointer_new (void)
{
  return (XrdScenePointer*) g_object_new (XRD_TYPE_SCENE_POINTER, 0);
}

static void
xrd_scene_pointer_finalize (GObject *gobject)
{
  XrdScenePointer *self = XRD_SCENE_POINTER (gobject);
  vkDestroyDescriptorPool (self->vertex_buffer->device,
                           self->descriptor_pool, NULL);

  g_object_unref (self->vertex_buffer);


  for (uint32_t eye = 0; eye < 2; eye++)
    g_object_unref (self->uniform_buffers[eye]);
}

gboolean
xrd_scene_pointer_init_descriptors (XrdScenePointer       *self,
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

void
xrd_scene_pointer_update_descriptors (XrdScenePointer *self)
{
  for (uint32_t eye = 0; eye < 2; eye++)
    {
      VkWriteDescriptorSet *write_descriptor_sets = (VkWriteDescriptorSet []) {
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = self->descriptor_sets[eye],
          .dstBinding = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .pBufferInfo = &(VkDescriptorBufferInfo) {
            .buffer = self->uniform_buffers[eye]->buffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE
          },
          .pTexelBufferView = NULL
        }
      };

      vkUpdateDescriptorSets (self->device->device,
                              1, write_descriptor_sets, 0, NULL);
    }
}

gboolean
xrd_scene_pointer_initialize (XrdScenePointer       *self,
                              GulkanDevice          *device,
                              VkDescriptorSetLayout *layout)
{
  self->device = device;

  gulkan_vertex_buffer_reset (self->vertex_buffer);

  graphene_vec4_t start;
  graphene_vec4_init (&start, 0, 0, -0.02f, 1);

  graphene_matrix_t identity;
  graphene_matrix_init_identity (&identity);

  gulkan_geometry_append_ray (self->vertex_buffer, &start, 40.0f, &identity);
  if (!gulkan_vertex_buffer_alloc_empty (self->vertex_buffer, device,
                                         k_unMaxTrackedDeviceCount))
    return FALSE;

  gulkan_vertex_buffer_map_array (self->vertex_buffer);

  for (uint32_t eye = 0; eye < 2; eye++)
    gulkan_uniform_buffer_allocate_and_map (self->uniform_buffers[eye],
                                            self->device, sizeof (float) * 16);

  if (!xrd_scene_pointer_init_descriptors (self, layout))
    return FALSE;

  xrd_scene_pointer_update_descriptors (self);

  return TRUE;
}

void
xrd_scene_pointer_update (XrdScenePointer    *self,
                          graphene_vec4_t    *start,
                          float               length)
{
  gulkan_vertex_buffer_reset (self->vertex_buffer);

  graphene_matrix_t identity;
  graphene_matrix_init_identity (&identity);

  gulkan_geometry_append_ray (self->vertex_buffer, start, length, &identity);
  gulkan_vertex_buffer_map_array (self->vertex_buffer);
}

void
xrd_scene_pointer_render (XrdScenePointer   *self,
                          EVREye             eye,
                          VkPipeline         pipeline,
                          VkPipelineLayout   pipeline_layout,
                          VkCommandBuffer    cmd_buffer,
                          graphene_matrix_t *vp)
{
  if (self->vertex_buffer->buffer == VK_NULL_HANDLE)
    return;

  vkCmdBindPipeline (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  graphene_matrix_t mvp;
  graphene_matrix_multiply (&self->model_matrix, vp, &mvp);

  /* Update matrix in uniform buffer */
  graphene_matrix_to_float (&mvp, self->uniform_buffers[eye]->data);

  vkCmdBindDescriptorSets (
    cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
   &self->descriptor_sets[eye], 0, NULL);

  gulkan_vertex_buffer_draw (self->vertex_buffer, cmd_buffer);
}
