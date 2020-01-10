/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-backend.h"

#include <gmodule.h>

#include "gxr-enums.h"
#include "gxr-config.h"
#include "gxr-context.h"

struct _GxrBackend
{
  GObject parent;
  GModule *module;
  GxrContext *(*context_new) (void);
};

G_DEFINE_TYPE (GxrBackend, gxr_backend, G_TYPE_OBJECT)

/* Backend singleton */
static GxrBackend *backend = NULL;

static void
gxr_backend_finalize (GObject *gobject);

static void
gxr_backend_class_init (GxrBackendClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gxr_backend_finalize;
}

GxrBackend *
gxr_backend_new_from_api (GxrApi api)
{
  gboolean supported = g_module_supported();
  if (!supported)
    {
      g_printerr ("Module loading not supported on this platform!\n");
      return NULL;
    }

  GxrBackend *self = (GxrBackend*) g_object_new (GXR_TYPE_BACKEND, 0);

  const gchar *plugin_dir = g_getenv ("GXR_BACKEND_DIR");
  if (!plugin_dir || !*plugin_dir)
    plugin_dir = BACKEND_DIR;

  gchar *func_name = NULL;
  gchar *module_name = NULL;
  switch (api)
    {
    case GXR_API_OPENVR:
      func_name = "openvr_context_new";
      module_name = "gxr-openvr";
      break;
    case GXR_API_OPENXR:
      func_name = "openxr_context_new";
      module_name = "gxr-openxr";
      break;
    }

  g_debug ("Load module %s from path %s\n", module_name, plugin_dir);
  gchar *module_path = g_module_build_path (plugin_dir, module_name);

  if (!g_file_test (module_path, G_FILE_TEST_EXISTS))
    {
      g_printerr ("The module file '%s' does not exist.\n\n"
                  "1. Local build\n"
                  "You don't have Gxr installed, try setting GXR_BACKEND_DIR to your build dir:\n\n"
                  "\t$ GXR_BACKEND_DIR=build/src/ ./build/examples/foo\n\n"
                  "2. System wide installtion\n"
                  "The '%s' module is missing in '%s'.\n"
                  "Contact your package maintainer.\n",
                  module_path, module_name, BACKEND_DIR);
      return NULL;
    }

  self->module = g_module_open (module_path, G_MODULE_BIND_LAZY);
  if (!self->module)
    {
      g_printerr ("Unable to load '%s' module.\n", module_name);
      return NULL;
    }

  if (!g_module_symbol (self->module, func_name,
                        (gpointer*) &self->context_new))
    {
      g_printerr ("Unable to get function reference: %s\n", g_module_error());
      g_module_close (self->module);
      return NULL;
    }

  return self;
}

GxrBackend *
gxr_backend_get_instance (GxrApi api)
{
  if (backend == NULL)
    backend = gxr_backend_new_from_api (api);
  return backend;
}

void
gxr_backend_shutdown (void)
{
  g_object_unref (backend);
}

static void
gxr_backend_init (GxrBackend *self)
{
  (void) self;
}

GxrContext *
gxr_backend_new_context (GxrBackend *self)
{
  return self->context_new();
}

static void
gxr_backend_finalize (GObject *gobject)
{
  GxrBackend *self = GXR_BACKEND (gobject);
  g_module_close (self->module);
  G_OBJECT_CLASS (gxr_backend_parent_class)->finalize (gobject);
}

