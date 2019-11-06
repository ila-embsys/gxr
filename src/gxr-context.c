/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-context.h"

struct _GxrContext
{
  GObject parent;
  GxrApi api;
};

#define GXR_DEFAULT_API GXR_API_OPENVR

G_DEFINE_TYPE (GxrContext, gxr_context, G_TYPE_OBJECT)

// singleton variable that can be set to NULL again when finalizing the context
static GxrContext *singleton = NULL;

static void
gxr_context_finalize (GObject *gobject);

static void
gxr_context_class_init (GxrContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gxr_context_finalize;
}

static void
gxr_context_init (GxrContext *self)
{
  const gchar *api_env = g_getenv ("GXR_API");
  if (g_strcmp0 (api_env, "openxr") == 0)
    self->api = GXR_API_OPENXR;
  else if (g_strcmp0 (api_env, "openvr") == 0)
    self->api = GXR_API_OPENVR;
  else
    self->api = GXR_DEFAULT_API;
}

GxrContext *
gxr_context_new (void)
{
  return (GxrContext*) g_object_new (GXR_TYPE_CONTEXT, 0);
}

GxrContext *
gxr_context_get_instance ()
{
  if (singleton == NULL)
    singleton = gxr_context_new ();

  return singleton;
}

static void
gxr_context_finalize (GObject *gobject)
{
  GxrContext *self = GXR_CONTEXT (gobject);
  (void) self;
  G_OBJECT_CLASS (gxr_context_parent_class)->finalize (gobject);
}

GxrApi
gxr_context_get_api (GxrContext *self)
{
  return self->api;
}
