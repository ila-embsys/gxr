/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-vulkan-model-manager.h"
#include "openvr-system.h"

G_DEFINE_TYPE (OpenVRVulkanModelManager, openvr_vulkan_model_manager, G_TYPE_OBJECT)

static void
openvr_vulkan_model_manager_finalize (GObject *gobject);

static void
openvr_vulkan_model_manager_class_init (OpenVRVulkanModelManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_vulkan_model_manager_finalize;
}

static void
openvr_vulkan_model_manager_init (OpenVRVulkanModelManager *self)
{
  self->model_content = g_hash_table_new_full (g_str_hash, g_str_equal,
                                               g_free, g_object_unref);
  memset (self->models, 0, sizeof (self->models));
}

OpenVRVulkanModelManager *
openvr_vulkan_model_manager_new (void)
{
  return (OpenVRVulkanModelManager*) g_object_new (OPENVR_TYPE_VULKAN_MODEL_MANAGER, 0);
}

static void
openvr_vulkan_model_manager_finalize (GObject *gobject)
{
  OpenVRVulkanModelManager *self = OPENVR_VULKAN_MODEL_MANAGER (gobject);
  g_hash_table_unref (self->model_content);

  for (uint32_t i = 0; i < G_N_ELEMENTS (self->models); i++)
    if (self->models[i] != NULL)
      g_object_unref (self->models[i]);
}

OpenVRVulkanModelContent*
_load_content (OpenVRVulkanModelManager *self,
               GulkanClient             *client,
               const char               *model_name)
{
  OpenVRVulkanModelContent *content;

  FencedCommandBuffer cmd_buffer;
  if (!gulkan_client_begin_res_cmd_buffer (client, &cmd_buffer))
    return NULL;
  content = openvr_vulkan_model_content_new ();
  if (!openvr_vulkan_model_content_load (content, client->device,
                                         cmd_buffer.cmd_buffer, model_name))
    return NULL;

  if (!gulkan_client_submit_res_cmd_buffer (client, &cmd_buffer))
    return NULL;

  g_hash_table_insert (self->model_content, g_strdup (model_name), content);

  return content;
}

void
openvr_vulkan_model_manager_load (OpenVRVulkanModelManager *self,
                                  GulkanClient             *client,
                                  TrackedDeviceIndex_t      device_id,
                                  VkDescriptorSetLayout     layout)
{
  gchar *model_name =
    openvr_system_get_device_string (
      device_id, ETrackedDeviceProperty_Prop_RenderModelName_String);

  OpenVRVulkanModelContent *content =
    g_hash_table_lookup (self->model_content, g_strdup (model_name));

  if (content == NULL)
    content = _load_content (self, client, model_name);

  if (content == NULL)
    {
      g_printerr ("Could not load content for model %s.\n", model_name);
      g_free (model_name);
      return;
    }

  OpenVRVulkanModel *model = openvr_vulkan_model_new ();
  if (!openvr_vulkan_model_initialize (model, content, client->device, layout))
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
