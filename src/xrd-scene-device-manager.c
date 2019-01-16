/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-device-manager.h"
#include "openvr-vulkan-model.h"
#include "openvr-system.h"
#include "openvr-math.h"

G_DEFINE_TYPE (XrdSceneDeviceManager, xrd_scene_device_manager, G_TYPE_OBJECT)

static void
xrd_scene_device_manager_finalize (GObject *gobject);

static void
xrd_scene_device_manager_class_init (XrdSceneDeviceManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_device_manager_finalize;
}

static void
xrd_scene_device_manager_init (XrdSceneDeviceManager *self)
{
  self->model_content = g_hash_table_new_full (g_str_hash, g_str_equal,
                                               g_free, g_object_unref);
  memset (self->models, 0, sizeof (self->models));
}

XrdSceneDeviceManager *
xrd_scene_device_manager_new (void)
{
  return (XrdSceneDeviceManager*) g_object_new (XRD_TYPE_SCENE_DEVICE_MANAGER, 0);
}

static void
xrd_scene_device_manager_finalize (GObject *gobject)
{
  XrdSceneDeviceManager *self = XRD_SCENE_DEVICE_MANAGER (gobject);
  g_hash_table_unref (self->model_content);

  for (uint32_t i = 0; i < G_N_ELEMENTS (self->models); i++)
    if (self->models[i] != NULL)
      g_object_unref (self->models[i]);

  for (TrackedDeviceIndex_t i = k_unTrackedDeviceIndex_Hmd + 1;
       i < k_unMaxTrackedDeviceCount; i++)
    if (self->pointers[i] != NULL)
      g_object_unref (self->pointers[i]);
}

OpenVRVulkanModel*
_load_content (XrdSceneDeviceManager *self,
               GulkanClient          *client,
               const char            *model_name)
{
  OpenVRVulkanModel *content;

  FencedCommandBuffer cmd_buffer;
  if (!gulkan_client_begin_res_cmd_buffer (client, &cmd_buffer))
    return NULL;
  content = openvr_vulkan_model_new ();
  if (!openvr_vulkan_model_load (content, client->device,
                                 cmd_buffer.cmd_buffer, model_name))
    return NULL;

  if (!gulkan_client_submit_res_cmd_buffer (client, &cmd_buffer))
    return NULL;

  g_hash_table_insert (self->model_content, g_strdup (model_name), content);

  return content;
}

void
xrd_scene_device_manager_add (XrdSceneDeviceManager *self,
                              GulkanClient          *client,
                              TrackedDeviceIndex_t   device_id,
                              VkDescriptorSetLayout *layout)
{
  gchar *model_name =
    openvr_system_get_device_string (
      device_id, ETrackedDeviceProperty_Prop_RenderModelName_String);

  OpenVRVulkanModel *content =
    g_hash_table_lookup (self->model_content, g_strdup (model_name));

  if (content == NULL)
    content = _load_content (self, client, model_name);

  if (content == NULL)
    {
      g_printerr ("Could not load content for model %s.\n", model_name);
      g_free (model_name);
      return;
    }

  XrdSceneDevice *model = xrd_scene_device_new ();
  if (!xrd_scene_device_initialize (model, content, client->device, layout))
    {
      g_print ("Unable to create Vulkan model from OpenVR model %s\n",
               model_name);
      g_object_unref (model);
      g_free (model_name);
      return;
    }

  g_free (model_name);

  self->models[device_id] = model;

  OpenVRContext *context = openvr_context_get_instance ();
  if (context->system->GetTrackedDeviceClass (device_id) ==
      ETrackedDeviceClass_TrackedDeviceClass_Controller)
    {
      if (self->pointers[device_id] == NULL)
        {
          self->pointers[device_id] = xrd_scene_pointer_new ();
          xrd_scene_pointer_initialize (self->pointers[device_id],
                                        client->device, layout);
        }
    }
}

void
xrd_scene_device_manager_remove (XrdSceneDeviceManager *self,
                                 TrackedDeviceIndex_t   device_id)
{
  g_object_unref (self->models[device_id]);
  self->models[device_id] = NULL;

  if (self->pointers[device_id] != NULL)
    {
      g_object_unref (self->pointers[device_id]);
      self->pointers[device_id] = NULL;
    }
}

void
xrd_scene_device_manager_render (XrdSceneDeviceManager *self,
                                 EVREye                 eye,
                                 VkCommandBuffer        cmd_buffer,
                                 VkPipeline             pipeline,
                                 VkPipelineLayout       layout,
                                 graphene_matrix_t     *vp)
{
  vkCmdBindPipeline (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  for (uint32_t i = 0; i < k_unMaxTrackedDeviceCount; i++)
    {
      if (!self->models[i])
        continue;

      TrackedDevicePose_t *pose = &self->device_poses[i];
      if (!pose->bPoseIsValid)
        continue;

      OpenVRContext *context = openvr_context_get_instance ();
      if (!context->system->IsInputAvailable () &&
          context->system->GetTrackedDeviceClass (i) ==
              ETrackedDeviceClass_TrackedDeviceClass_Controller)
        continue;

      graphene_matrix_t mvp;
      graphene_matrix_init_from_matrix (&mvp, &self->device_mats[i]);

      graphene_matrix_multiply (&mvp, vp, &mvp);

      xrd_scene_device_draw (self->models[i], eye, cmd_buffer, layout, &mvp);
    }
}

void
xrd_scene_device_manager_update_poses (XrdSceneDeviceManager *self,
                                       graphene_matrix_t     *mat_head_pose)
{
  OpenVRContext *context = openvr_context_get_instance ();
  context->compositor->WaitGetPoses (self->device_poses,
                                     k_unMaxTrackedDeviceCount, NULL, 0);

  for (uint32_t i = 0; i < k_unMaxTrackedDeviceCount; ++i)
    {
      if (!self->device_poses[i].bPoseIsValid)
        continue;

      openvr_math_matrix34_to_graphene (
        &self->device_poses[i].mDeviceToAbsoluteTracking,
        &self->device_mats[i]);

      if (context->system->GetTrackedDeviceClass (i) ==
          ETrackedDeviceClass_TrackedDeviceClass_Controller &&
          context->system->IsTrackedDeviceConnected (i))
        {
          if (self->pointers[i] != NULL)
            openvr_math_matrix34_to_graphene (
              &self->device_poses[i].mDeviceToAbsoluteTracking,
              &self->pointers[i]->model_matrix);
        }
    }

  if (self->device_poses[k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
    {
      *mat_head_pose = self->device_mats[k_unTrackedDeviceIndex_Hmd];
      graphene_matrix_inverse (mat_head_pose, mat_head_pose);
    }
}

void
xrd_scene_device_manager_render_pointers (XrdSceneDeviceManager *self,
                                          EVREye                 eye,
                                          VkCommandBuffer        cmd_buffer,
                                          VkPipeline             pipeline,
                                          VkPipelineLayout       pipeline_layout,
                                          graphene_matrix_t     *vp)
{
  OpenVRContext *context = openvr_context_get_instance ();
  if (context->system->IsInputAvailable ())
    for (TrackedDeviceIndex_t i = k_unTrackedDeviceIndex_Hmd + 1;
         i < k_unMaxTrackedDeviceCount; i++)
      if (self->pointers[i] != NULL)
        xrd_scene_pointer_render (self->pointers[i], eye, pipeline,
                                  pipeline_layout, cmd_buffer, vp);
}
