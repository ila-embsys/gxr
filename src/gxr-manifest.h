/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_MANIFEST_H_
#define GXR_MANIFEST_H_

#include <gio/gio.h>
#include <glib-object.h>

/**
 * GxrBindingType:
 * @GXR_BINDING_TYPE_UNKNOWN: An unknown binding type.
 * @GXR_BINDING_TYPE_POSE: A pose binding type.
 * @GXR_BINDING_TYPE_BOOLEAN: A pose boolean type.
 * @GXR_BINDING_TYPE_FLOAT: A pose float type.
 * @GXR_BINDING_TYPE_VEC2: A pose vec2 type.
 * @GXR_BINDING_TYPE_HAPTIC: A pose haptic type.
 *
 * The type of GxrBinding.
 *
 **/
typedef enum
{
  GXR_BINDING_TYPE_UNKNOWN,
  GXR_BINDING_TYPE_POSE,
  GXR_BINDING_TYPE_BOOLEAN,
  GXR_BINDING_TYPE_FLOAT,
  GXR_BINDING_TYPE_VEC2,
  GXR_BINDING_TYPE_HAPTIC
} GxrBindingType;

/**
 * GxrBindingMode:
 * @GXR_BINDING_MODE_NONE: None.
 * @GXR_BINDING_MODE_UNKNOWN: Unknown.
 * @GXR_BINDING_MODE_BUTTON: Button.
 * @GXR_BINDING_MODE_TRACKPAD: Trackpad.
 * @GXR_BINDING_MODE_ANALOG_STICK: Thumbstick.
 *
 * The mode of the GxrBinding.
 *
 **/
typedef enum
{
  GXR_BINDING_MODE_NONE,
  GXR_BINDING_MODE_UNKNOWN,
  GXR_BINDING_MODE_BUTTON,
  GXR_BINDING_MODE_TRACKPAD,
  GXR_BINDING_MODE_ANALOG_STICK,
} GxrBindingMode;

/**
 * GxrBindingComponent:
 * @GXR_BINDING_COMPONENT_NONE: None.
 * @GXR_BINDING_COMPONENT_UNKNOWN: Unknown.
 * @GXR_BINDING_COMPONENT_CLICK: Click.
 * @GXR_BINDING_COMPONENT_PULL: Pull.
 * @GXR_BINDING_COMPONENT_POSITION: Position.
 * @GXR_BINDING_COMPONENT_TOUCH: Touch.
 * @GXR_BINDING_COMPONENT_FORCE: Force.
 *
 * The component of the GxrBindingPath.
 *
 **/
typedef enum
{
  GXR_BINDING_COMPONENT_NONE,
  GXR_BINDING_COMPONENT_UNKNOWN,
  GXR_BINDING_COMPONENT_CLICK,
  GXR_BINDING_COMPONENT_PULL,
  GXR_BINDING_COMPONENT_POSITION,
  GXR_BINDING_COMPONENT_TOUCH,
  GXR_BINDING_COMPONENT_FORCE
} GxrBindingComponent;

typedef struct
{
  gchar         *name;
  GxrBindingType type;
} GxrActionManifestEntry;

typedef struct
{
  GxrBindingComponent component;
  gchar              *path;
  GxrBindingMode      mode;
} GxrBindingPath;

typedef struct
{
  GxrActionManifestEntry *action;
  GSList                 *input_paths;
} GxrBinding;

typedef struct
{
  gchar *filename;

  /* list of GxrBinding */
  GSList *gxr_bindings;

  /* Only set for OpenXR manifest */
  gchar *interaction_profile;
} GxrBindingManifest;

G_BEGIN_DECLS

#define GXR_TYPE_MANIFEST gxr_manifest_get_type ()
G_DECLARE_FINAL_TYPE (GxrManifest, gxr_manifest, GXR, MANIFEST, GObject)

/**
 * gxr_manifest_new:
 * @resource_path: The base path of the resources.
 * @manifest_name: The name of the manifest file.
 * @returns: A newly allocated #GxrManifest.
 *
 * Creates a new manifest from a resource path and manifest name.
 */
GxrManifest *
gxr_manifest_new (const char *resource_path, const char *manifest_name);

/**
 * gxr_manifest_load_actions:
 * @self: A #GxrManifest.
 * @action_stream: The action stream.
 * @returns: `TRUE` if the actions were loaded successfully, `FALSE` otherwise.
 *
 * Loads the actions from the given stream.
 */
gboolean
gxr_manifest_load_actions (GxrManifest *self, GInputStream *action_stream);

/**
 * gxr_manifest_load_bindings:
 * @self: A #GxrManifest.
 * @resource_path: The base path of the resources.
 * @returns: `TRUE` if the bindings were loaded successfully, `FALSE` otherwise.
 *
 * Loads the bindings from the given resource path.
 */
gboolean
gxr_manifest_load_bindings (GxrManifest *self, const char *resource_path);

/**
 * gxr_manifest_get_binding_filenames:
 * @self: A #GxrManifest.
 * @returns: (element-type GString) (transfer container): A newly allocated #GSList containing the binding filenames.
 *
 * Gets the binding filenames.
 */
GSList *
gxr_manifest_get_binding_filenames (GxrManifest *self);

/**
 * gxr_manifest_get_binding_manifests:
 * @manifest: A #GxrManifest.
 * @returns: (element-type GxrBindingManifest) (transfer container): A newly allocated #GSList containing the binding manifests.
 *
 * Gets the binding manifests.
 */
GSList *
gxr_manifest_get_binding_manifests (GxrManifest *manifest);

G_END_DECLS

#endif /* GXR_MANIFEST_H_ */
