#pragma once

#include <glib.h>
#include <openvr_capi.h>

gboolean
openvr_init_fn_table (const char *type, intptr_t *ret);

#define INIT_FN_TABLE(target, type) \
{ \
  intptr_t ptr = 0; \
  gboolean ret = openvr_init_fn_table (IVR##type##_Version, &ptr); \
  if (!ret || ptr == 0) \
    return false; \
  target = (struct VR_IVR##type##_FnTable*) ptr; \
  return true; \
}
