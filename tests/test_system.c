/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <openvr-glib.h>

#include "openvr-system.h"

int main(int argc, char *argv[]) {
  OpenVRSystem * system = openvr_system_new ();

  g_assert (openvr_system_init_overlay (system));

  g_assert (openvr_system_is_available ());
  g_assert (openvr_system_is_compositor_available ());
  g_assert (openvr_system_is_overlay_available ());

  g_object_unref (system);

  return 0;
}
