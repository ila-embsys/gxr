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
  openvr_system_print_device_info ();
}

int
main ()
{
  _test_minimal ();

  return 0;
}
