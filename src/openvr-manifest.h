/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_MANIFEST_H_
#define OPENVR_GLIB_MANIFEST_H_

#include <glib-object.h>
#include <json-glib/json-glib.h>

typedef enum
{
  BINDING_TYPE_UNKNOWN,
  BINDING_TYPE_POSE,
  BINDING_TYPE_BOOLEAN,
  BINDING_TYPE_VEC2,
  BINDING_TYPE_HAPTIC
} OpenVRBindingType;

typedef enum
{
  BINDING_MODE_UNKNOWN,
  BINDING_MODE_BUTTON,
  BINDING_MODE_TRACKPAD,
  BINDING_MODE_JOYSTICK
} OpenVRBindingMode;

typedef enum
{
  BINDING_COMPONENT_UNKNOWN,
  BINDING_COMPONENT_CLICK,
  BINDING_COMPONENT_POSITION
} OpenVRBindingComponent;

G_BEGIN_DECLS

#define OPENVR_TYPE_MANIFEST openvr_manifest_get_type ()
G_DECLARE_FINAL_TYPE (OpenVRManifest, openvr_manifest, OPENVR, MANIFEST, GObject)

OpenVRManifest *openvr_manifest_new (void);

gboolean
openvr_manifest_load (OpenVRManifest *self,
                      GInputStream *action_stream,
                      GInputStream *binding_stream);

G_END_DECLS

#endif /* OPENVR_GLIB_MANIFEST_H_ */
