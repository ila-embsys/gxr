/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <openvr-glib.h>

#include "openvr-system.h"

int main(int argc, char *argv[]) {
  g_print("testing system class.\n");

  OpenVRSystem * system = openvr_system_new ();

  gboolean ret = openvr_system_init_overlay (system);

  g_assert (ret);

  g_assert (openvr_system_is_available ());

  g_assert (openvr_system_is_compositor_available ());

  openvr_system_print_device_info (system);

  g_object_unref (system);

  return 0;
}
