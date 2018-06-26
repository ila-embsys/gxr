#pragma once

#include <glib.h>
#include <openvr_capi.h>

gboolean
openvr_init_fn_table (const char *type, intptr_t *ret);
