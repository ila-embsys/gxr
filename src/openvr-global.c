#include "openvr-global.h"
#include "openvr_capi_global.h"

#include <glib/gprintf.h>

uint64_t ButtonMaskFromId( EVRButtonId id ) { return 1ull << id; }

gboolean
openvr_init_fn_table (const char *type, intptr_t *ret)
{
  EVRInitError error;
  char fn_table_name[128];
  g_sprintf (fn_table_name, "FnTable:%s", type);

  *ret = VR_GetGenericInterface (fn_table_name, &error);

  if (error != EVRInitError_VRInitError_None)
    {
      g_printerr ("VR_GetGenericInterface returned error %s: %s\n",
                  VR_GetVRInitErrorAsSymbol (error),
                  VR_GetVRInitErrorAsEnglishDescription (error));
      return FALSE;
    }

  return TRUE;
}
