/*
 * Xrd GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-window.h"
#include <gulkan-geometry.h>
#include <gulkan-descriptor-set.h>

G_DEFINE_TYPE (XrdSceneWindow, xrd_scene_window, G_TYPE_OBJECT)

static void
xrd_scene_window_finalize (GObject *gobject);

static void
xrd_scene_window_class_init (XrdSceneWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_window_finalize;
}

static void
xrd_scene_window_init (XrdSceneWindow *self)
{
  self->descriptor_pool = VK_NULL_HANDLE;

  graphene_matrix_init_identity (&self->model_matrix);

  self->vertex_buffer = gulkan_vertex_buffer_new ();
  self->scale = 1.0f;

  for (uint32_t eye = 0; eye < 2; eye++)
    self->uniform_buffers[eye] = gulkan_uniform_buffer_new ();

  self->scene_sampler = VK_NULL_HANDLE;

  memset (self->descriptor_sets, 0, sizeof (self->descriptor_sets));
}

XrdSceneWindow *
xrd_scene_window_new (void)
{
  return (XrdSceneWindow*) g_object_new (XRD_TYPE_SCENE_WINDOW, 0);
}

static void
xrd_scene_window_finalize (GObject *gobject)
{
  XrdSceneWindow *self = XRD_SCENE_WINDOW (gobject);
  g_object_unref (self->cat_texture);

  vkDestroySampler (self->device->device, self->scene_sampler, NULL);
  vkDestroyDescriptorPool (self->device->device, self->descriptor_pool, NULL);

  g_object_unref (self->vertex_buffer);

  for (uint32_t eye = 0; eye < 2; eye++)
    g_object_unref (self->uniform_buffers[eye]);
}

bool
xrd_scene_window_init_texture (XrdSceneWindow *self,
                               GulkanDevice   *device,
                               VkCommandBuffer cmd_buffer,
                               GdkPixbuf      *pixbuf)
{
  self->device = device;

  uint32_t mip_levels;

  self->cat_texture = gulkan_texture_new_from_pixbuf_mipmapped (
      device, cmd_buffer, pixbuf,
      &mip_levels, VK_FORMAT_R8G8B8A8_UNORM);

  gulkan_texture_transfer_layout_mips (self->cat_texture,
                                       device,
                                       cmd_buffer,
                                       mip_levels,
                                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  VkSamplerCreateInfo sampler_info = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .anisotropyEnable = VK_TRUE,
    .maxAnisotropy = 16.0f,
    .minLod = 0.0f,
    .maxLod = (float) mip_levels
  };

  vkCreateSampler (device->device, &sampler_info, NULL, &self->scene_sampler);

  return true;
}

void _append_plane (GulkanVertexBuffer *vbo,
                    float x, float y, float z, float scale)
{
  graphene_matrix_t mat_scale;
  graphene_matrix_init_scale (&mat_scale, scale, scale, scale);

  graphene_point3d_t translation = { x, y, z };
  graphene_matrix_t mat_translation;
  graphene_matrix_init_translate (&mat_translation, &translation);

  graphene_matrix_t mat;
  graphene_matrix_multiply (&mat_scale, &mat_translation, &mat);

  gulkan_geometry_append_plane (vbo, &mat);
}

gboolean
xrd_scene_window_init_descriptors (XrdSceneWindow        *self,
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
xrd_scene_window_update_descriptors (XrdSceneWindow *self)
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
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = self->descriptor_sets[eye],
          .dstBinding = 1,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .pImageInfo = &(VkDescriptorImageInfo) {
            .sampler = self->scene_sampler,
            .imageView = self->cat_texture->image_view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
          },
          .pBufferInfo = NULL,
          .pTexelBufferView = NULL
        }
      };

      vkUpdateDescriptorSets (self->device->device,
                              2, write_descriptor_sets, 0, NULL);
    }
}

gboolean
xrd_scene_window_initialize (XrdSceneWindow        *self,
                             GulkanDevice          *device,
                             VkDescriptorSetLayout *layout)
{
  /* TODO: Require device in constructor */
  self->device = device;

  _append_plane (self->vertex_buffer, 0, 0, 0, 1.0f);
  if (!gulkan_vertex_buffer_alloc_array (self->vertex_buffer, self->device))
    return FALSE;

  /* Create uniform buffer to hold a matrix per eye */
  for (uint32_t eye = 0; eye < 2; eye++)
    gulkan_uniform_buffer_allocate_and_map (self->uniform_buffers[eye],
                                            self->device, sizeof (float) * 16);

  if (!xrd_scene_window_init_descriptors (self, layout))
    return FALSE;

  xrd_scene_window_update_descriptors (self);

  return TRUE;
}

void
_update_model_matrix (XrdSceneWindow *self)
{
  graphene_matrix_init_scale (&self->model_matrix,
                              self->scale, self->scale, self->scale);
  graphene_matrix_translate (&self->model_matrix, &self->position);
}

void
xrd_scene_window_set_scale (XrdSceneWindow *self, float scale)
{
  self->scale = scale;
  _update_model_matrix (self);
}

void
xrd_scene_window_set_position (XrdSceneWindow     *self,
                               graphene_point3d_t *position)
{
  graphene_point3d_init_from_point (&self->position, position);
  _update_model_matrix (self);
}

void
xrd_scene_window_draw (XrdSceneWindow    *self,
                       EVREye             eye,
                       VkPipeline         pipeline,
                       VkPipelineLayout   pipeline_layout,
                       VkCommandBuffer    cmd_buffer,
                       graphene_matrix_t *vp)
{
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