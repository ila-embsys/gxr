#include <glib.h>
#include <openvr-glib.h>

#include "openvr-system.h"

int main(int argc, char *argv[]) {
  g_print("testing system class.\n");

  OpenVRSystem * system = openvr_system_new ();

  gboolean ret = openvr_system_init_overlay (system);

  g_assert (ret);

  g_object_unref (system);

  return 0;
}
