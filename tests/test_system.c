/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <openvr-glib.h>
#include <openvr_capi.h>
#include "openvr_capi_global.h"
#include "openvr-system.h"

void
_test_minimal ()
{
  OpenVRSystem * system = openvr_system_new ();
  openvr_system_print_device_info (system);
  g_object_unref (system);
}

int
main (int argc, char *argv[])
{
  _test_minimal ();

  return 0;
}
