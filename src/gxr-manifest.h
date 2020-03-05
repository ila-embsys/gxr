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

/**
 * GxrBindingType:
 * @BINDING_TYPE_UNKNOWN: An unknown binding type.
 * @BINDING_TYPE_POSE: A pose binding type.
 * @BINDING_TYPE_BOOLEAN: A pose boolean type.
 * @BINDING_TYPE_FLOAT: A pose float type.
 * @BINDING_TYPE_VEC2: A pose vec2 type.
 * @BINDING_TYPE_HAPTIC: A pose haptic type.
 *
 * The type of GxrBinding.
 *
 **/
typedef enum
{
  BINDING_TYPE_UNKNOWN,
  BINDING_TYPE_POSE,
  BINDING_TYPE_BOOLEAN,
  BINDING_TYPE_FLOAT,
  BINDING_TYPE_VEC2,
  BINDING_TYPE_HAPTIC
} GxrBindingType;

/**
 * GxrBindingMode:
 * @BINDING_MODE_NONE: A digital action.
 * @BINDING_MODE_UNKNOWN: A digital action constructed from float thresholds.
 * @BINDING_MODE_BUTTON: An analog action with floats x,y.
 * @BINDING_MODE_TRACKPAD: An analog action.
 * @BINDING_MODE_JOYSTICK: A pose action.
 *
 * The mode of the GxrBinding.
 *
 **/
typedef enum
{
  BINDING_MODE_NONE,
  BINDING_MODE_UNKNOWN,
  BINDING_MODE_BUTTON,
  BINDING_MODE_TRACKPAD,
  BINDING_MODE_JOYSTICK,
} GxrBindingMode;

/**
 * GxrBindingComponent:
 * @BINDING_COMPONENT_NONE: None.
 * @BINDING_COMPONENT_UNKNOWN: Unknown.
 * @BINDING_COMPONENT_CLICK: Click.
 * @BINDING_COMPONENT_PULL: Pull.
 * @BINDING_COMPONENT_POSITION: Position.
 * @BINDING_COMPONENT_TOUCH: Touch.
 *
 * The component of the GxrBindingInputPath.
 *
 **/
typedef enum
{
  BINDING_COMPONENT_NONE,
  BINDING_COMPONENT_UNKNOWN,
  BINDING_COMPONENT_CLICK,
  BINDING_COMPONENT_PULL,
  BINDING_COMPONENT_POSITION,
  BINDING_COMPONENT_TOUCH
} GxrBindingComponent;

typedef struct
{
  GxrBindingComponent component;
  gchar *path;
} GxrBindingInputPath;

typedef struct
{
  GxrBindingType binding_type;
  GList *input_paths;
  GxrBindingMode mode;
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
