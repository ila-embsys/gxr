/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-device-manager.h"
#include "xrd-scene-model.h"

#include <gxr.h>

struct _XrdSceneDeviceManager
{
  GObject parent;

  GHashTable *models; // char* -> XrdSceneModel
  GHashTable *devices; // int -> XrdSceneDevice
};

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
  self->models = g_hash_table_new_full (g_str_hash, g_str_equal,
                                        g_free, g_object_unref);
  self->devices = g_hash_table_new_full (g_int_hash, g_int_equal,
                                         g_free, g_object_unref);
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
  g_hash_table_unref (self->models);
  g_hash_table_unref (self->devices);
}

static XrdSceneModel*
_load_content (XrdSceneDeviceManager *self,
               GxrContext            *context,
               const char            *model_name)
{
  XrdSceneModel *content = xrd_scene_model_new ();
  if (!xrd_scene_model_load (content, context, model_name))
    return NULL;

  g_hash_table_insert (self->models, g_strdup (model_name), content);

  return content;
}

static void
_insert_at_key (GHashTable *table, uint32_t key, gpointer value)
{
  gint *keyp = g_new0 (gint, 1);
  *keyp = (gint) key;
  g_hash_table_insert (table, keyp, value);
}

gboolean
xrd_scene_device_manager_add (XrdSceneDeviceManager *self,
                              GxrContext            *context,
                              uint32_t               device_id,
                              gchar                 *model_name,
                              bool                   is_controller,
                              VkDescriptorSetLayout *layout)
{
  XrdSceneModel *content =
    g_hash_table_lookup (self->models, g_strdup (model_name));

  if (content == NULL)
    content = _load_content (self, context, model_name);

  if (content == NULL)
    {
      g_printerr ("Could not load content for model %s.\n", model_name);
      return FALSE;
    }

  XrdSceneDevice *device = xrd_scene_device_new ();
  if (!xrd_scene_device_initialize (device, content, layout))
    {
      g_print ("Unable to create Vulkan model from Gxr model %s\n",
               model_name);
      g_object_unref (device);
      return FALSE;
    }

  xrd_scene_device_set_is_controller (device, is_controller);

  _insert_at_key (self->devices, device_id, device);

  return TRUE;
}

void
xrd_scene_device_manager_remove (XrdSceneDeviceManager *self,
                                 uint32_t               device_id)
{
  g_hash_table_remove (self->devices, &device_id);
}

void
xrd_scene_device_manager_render (XrdSceneDeviceManager *self,
                                 gboolean               input_available,
                                 GxrEye                 eye,
                                 VkCommandBuffer        cmd_buffer,
                                 VkPipeline             pipeline,
                                 VkPipelineLayout       layout,
                                 graphene_matrix_t     *vp)
{
  vkCmdBindPipeline (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  GList *devices = g_hash_table_get_values (self->devices);
  for (GList *l = devices; l; l = l->next)
    if (input_available || !xrd_scene_device_is_controller (l->data))
      xrd_scene_device_draw (l->data, eye, cmd_buffer, layout, vp);
}

void
xrd_scene_device_manager_update_poses (XrdSceneDeviceManager *self,
                                       GxrPose               *poses)
{
  GList *device_keys = g_hash_table_get_keys (self->devices);
  for (GList *l = device_keys; l; l = l->next)
    {
      gint *key = l->data;
      uint32_t i = (uint32_t) *key;

      XrdSceneDevice *device = g_hash_table_lookup (self->devices, &i);
      xrd_scene_device_set_is_pose_valid (device, poses[i].is_valid);

      if (!poses[i].is_valid)
        continue;

      XrdSceneObject *obj = XRD_SCENE_OBJECT (device);
      xrd_scene_object_set_transformation_direct (obj, &poses[i].transformation);
    }
}

