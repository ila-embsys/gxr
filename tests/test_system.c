/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <openvr-glib.h>

#include "openvr-system.h"

int main(int argc, char *argv[]) {
  OpenVRSystem * system = openvr_system_new ();

  g_assert (openvr_system_init_overlay (system));

  g_assert (openvr_system_is_available (system));
  g_assert (openvr_system_is_installed ());

  g_object_unref (system);

  return 0;
}
