/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-context.h"
#include "gxr-config.h"

#include "openvr-system.h"
#include "openxr-context.h"
#include "openvr-context.h"

typedef struct _GxrContextPrivate
{
  GObject parent;
  GxrApi api;
} GxrContextPrivate;

#ifdef GXR_HAS_OPENVR
  #define GXR_DEFAULT_API GXR_API_OPENVR
#else
  #define GXR_DEFAULT_API GXR_API_OPENXR
#endif

G_DEFINE_TYPE_WITH_PRIVATE (GxrContext, gxr_context, G_TYPE_OBJECT)

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

static GxrApi
_get_api_from_env ()
{
  const gchar *api_env = g_getenv ("GXR_API");
  if (g_strcmp0 (api_env, "openxr") == 0)
    return GXR_API_OPENXR;
  else if (g_strcmp0 (api_env, "openvr") == 0)
    return GXR_API_OPENVR;
  else
    return GXR_DEFAULT_API;
}

static GxrContext*
_new_context_from_env ()
{
  GxrApi api = _get_api_from_env ();
  switch (api)
    {
#ifdef GXR_HAS_OPENVR
      case GXR_API_OPENVR:
        return GXR_CONTEXT (openvr_context_new ());
#endif
#ifdef GXR_HAS_OPENXR
      case GXR_API_OPENXR:
        return GXR_CONTEXT (openxr_context_new ());
#endif
      default:
        g_printerr ("ERROR: Could not init context: GXR API not supported.\n");
        return NULL;
    }
}

static void
gxr_context_init (GxrContext *self)
{
  GxrContextPrivate *priv = gxr_context_get_instance_private (self);
  priv->api = _get_api_from_env ();
}

GxrContext *
gxr_context_get_instance ()
{
  if (singleton == NULL)
    singleton = _new_context_from_env ();

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
  GxrContextPrivate *priv = gxr_context_get_instance_private (self);
  return priv->api;
}

gboolean
gxr_context_get_head_pose (graphene_matrix_t *pose)
{
  GxrContext *self = gxr_context_get_instance ();
  GxrContextPrivate *priv = gxr_context_get_instance_private (self);
  switch (priv->api)
    {
#ifdef GXR_HAS_OPENVR
    case GXR_API_OPENVR:
      return openvr_system_get_hmd_pose (pose);
#endif
#ifdef GXR_HAS_OPENXR
    case GXR_API_OPENXR:
      return openxr_context_get_head_pose (OPENXR_CONTEXT (self), pose);
#endif
    default:
      g_warning ("gxr_context_get_head_pose not supported by backend.\n");
      return FALSE;
  };
  return FALSE;
}

void
gxr_context_get_frustum_angles (GxrEye eye,
                                float *left, float *right,
                                float *top, float *bottom)
{
  GxrContext *self = gxr_context_get_instance ();
  GxrContextPrivate *priv = gxr_context_get_instance_private (self);
  switch (priv->api)
    {
    case GXR_API_OPENVR:
#ifdef GXR_HAS_OPENVR
      return openvr_system_get_frustum_angles (eye, left, right, top, bottom);
#endif
    /* TODO: Implement for OpenXR*/
    default: {
      (void) eye;
      *left = 1, *right = 1, *top = 1, *bottom = 1;
      g_warning ("gxr_context_get_frustum_angles not supported by backend.\n");
      return;
    }
  };
}

gboolean
gxr_context_is_input_available (void)
{
  GxrContext *self = gxr_context_get_instance ();
  GxrContextPrivate *priv = gxr_context_get_instance_private (self);
  switch (priv->api)
    {
    case GXR_API_OPENVR:
#ifdef GXR_HAS_OPENVR
      return openvr_system_is_input_available ();
#endif
    /* TODO: Implement for OpenXR*/
    default:
      return TRUE;
  };
}