/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-device-manager.h"
#include "openvr-vulkan-model.h"
#include "openvr-system.h"

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
xrd_scene_device_manager_load (XrdSceneDeviceManager *self,
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
}
