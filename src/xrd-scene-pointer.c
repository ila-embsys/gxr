/*
 * Xrd GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-pointer.h"
#include "gulkan-geometry.h"

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
  self->pointer_vbo = gulkan_vertex_buffer_new ();
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
  g_object_unref (self->pointer_vbo);
}

void
xrd_scene_pointer_update (XrdScenePointer    *self,
                          GulkanDevice       *device,
                          TrackedDevicePose_t device_poses[MAX_TRACKED_DEVICES],
                          graphene_matrix_t   device_mats[MAX_TRACKED_DEVICES])
{
  gulkan_vertex_buffer_reset (self->pointer_vbo);

  for (TrackedDeviceIndex_t i = k_unTrackedDeviceIndex_Hmd + 1;
       i < k_unMaxTrackedDeviceCount; i++)
    {
      OpenVRContext *context = openvr_context_get_instance ();
      if (!context->system->IsTrackedDeviceConnected (i))
        continue;

      if (context->system->GetTrackedDeviceClass (i) !=
          ETrackedDeviceClass_TrackedDeviceClass_Controller)
        continue;

      if (!device_poses[i].bPoseIsValid)
        continue;

      graphene_matrix_t *mat = &device_mats[i];

      graphene_vec4_t center;
      graphene_vec4_init (&center, 0, 0, 0, 1);

      gulkan_geometry_append_axes (self->pointer_vbo, &center, 0.05f, mat);

      graphene_vec4_t start;
      graphene_vec4_init (&start, 0, 0, -0.02f, 1);
      gulkan_geometry_append_ray (self->pointer_vbo, &start, 40.0f, mat);
    }

  /* We didn't find any controller devices */
  if (self->pointer_vbo->array->len == 0)
    return;

  /* Setup the vbo the first time */
  if (self->pointer_vbo->buffer == VK_NULL_HANDLE)
    if (!gulkan_vertex_buffer_alloc_empty (self->pointer_vbo,
                                           device,
                                           k_unMaxTrackedDeviceCount))
      return;

  gulkan_vertex_buffer_map_array (self->pointer_vbo);
}

void
xrd_scene_pointer_render_pointer (XrdScenePointer *self,
                                  VkPipeline       pipeline,
                                  VkCommandBuffer  cmd_buffer)
{
  if (self->pointer_vbo->buffer == VK_NULL_HANDLE)
    return;

  vkCmdBindPipeline (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  gulkan_vertex_buffer_draw (self->pointer_vbo, cmd_buffer);
}
