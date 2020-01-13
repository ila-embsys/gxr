/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_MANIFEST_H_
#define GXR_MANIFEST_H_

#include <glib-object.h>
#include <gio/gio.h>

typedef enum
{
  BINDING_TYPE_UNKNOWN,
  BINDING_TYPE_POSE,
  BINDING_TYPE_BOOLEAN,
  BINDING_TYPE_VEC2,
  BINDING_TYPE_HAPTIC
} GxrManifestBindingType;

typedef enum
{
  BINDING_MODE_NONE,
  BINDING_MODE_UNKNOWN,
  BINDING_MODE_BUTTON,
  BINDING_MODE_TRACKPAD,
  BINDING_MODE_JOYSTICK,
} GxrManifestBindingMode;

typedef enum
{
  BINDING_COMPONENT_NONE,
  BINDING_COMPONENT_UNKNOWN,
  BINDING_COMPONENT_CLICK,
  BINDING_COMPONENT_POSITION,
  BINDING_COMPONENT_TOUCH
} GxrManifestBindingComponent;

typedef struct
{
  GxrManifestBindingComponent component;
  gchar *path;
} GxrBindingInputPath;

typedef struct
{
  GxrManifestBindingType binding_type;
  GList *input_paths;
  GxrManifestBindingMode mode;
} GxrBinding;

G_BEGIN_DECLS

#define GXR_TYPE_MANIFEST gxr_manifest_get_type ()
G_DECLARE_FINAL_TYPE (GxrManifest, gxr_manifest, GXR, MANIFEST, GObject)

GxrManifest *gxr_manifest_new (void);

gboolean
gxr_manifest_load (GxrManifest *self,
                   GInputStream *action_stream,
                   GInputStream *binding_stream);

gchar *
gxr_manifest_get_interaction_profile (GxrManifest *self);

GHashTable *
gxr_manifest_get_hash_table (GxrManifest *self);

int
gxr_manifest_get_num_inputs (GxrManifest *self);

G_END_DECLS

#endif /* GXR_MANIFEST_H_ */
