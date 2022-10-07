/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gxr.h>

int
main ()
{
  GxrManifest *manifest = gxr_manifest_new ("/res/bindings", "actions.json");
  g_assert (manifest);
  g_object_unref (manifest);
}
