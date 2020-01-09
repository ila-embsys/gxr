/*
 * gxresktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-overlay-model.h"

typedef struct  _GxrOverlayModelPrivate
{
  GObject parent;
  GxrOverlay *overlay;
} GxrOverlayModelPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GxrOverlayModel, gxr_overlay_model, G_TYPE_OBJECT)

static void
gxr_overlay_model_finalize (GObject *gobject);

static void
gxr_overlay_model_class_init (GxrOverlayModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gxr_overlay_model_finalize;
}

static void
gxr_overlay_model_init (GxrOverlayModel *self)
{
  GxrOverlayModelPrivate *priv = gxr_overlay_model_get_instance_private (self);
  priv->overlay = gxr_overlay_new ();
}

static void
_destroy_pixels_cb (guchar *pixels, gpointer unused)
{
  (void) unused;
  g_free (pixels);
}

static GdkPixbuf *
_create_empty_pixbuf (uint32_t width, uint32_t height)
{
  guchar *pixels = (guchar*) g_malloc (sizeof (guchar) * height * width * 4);
  memset (pixels, 0, height * width * 4 * sizeof (guchar));
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB,
                                                TRUE, 8, (int) width,
                                                (int) height,
                                                4 * (int) width,
                                                _destroy_pixels_cb, NULL);
  return pixbuf;
}

GxrOverlayModel *
gxr_overlay_model_new (gchar* key, gchar* name)
{
  GxrOverlayModel *self = (GxrOverlayModel*) g_object_new (GXR_TYPE_OVERLAY_MODEL, 0);

  if (!gxr_overlay_model_initialize (self, key, name))
    return NULL;

  return self;
}

/*
 * TODO: Not sure how the parent _new constructor can be shared with
 *       GxrOverlayPointer. Casting to the child class is not allowed.
 *       As a workaround I am introducting this initialization method.
 */

gboolean
gxr_overlay_model_initialize (GxrOverlayModel *self, gchar* key, gchar* name)
{
  GxrOverlayModelPrivate *priv = gxr_overlay_model_get_instance_private (self);
  GxrOverlay *overlay = priv->overlay;

  if (!gxr_overlay_create (overlay, key, name))
    return FALSE;

  if (!gxr_overlay_is_valid (overlay))
    {
      g_printerr ("Model overlay %s %s unavailable.\n", key, name);
      return FALSE;
    }

  GdkPixbuf *pixbuf = _create_empty_pixbuf (10, 10);
  if (pixbuf == NULL)
    return FALSE;

  /*
   * Overlay needs a texture to be set to show model
   * See https://github.com/ValveSoftware/openvr/issues/496
   */
  gxr_overlay_set_gdk_pixbuf_raw (overlay, pixbuf);
  g_object_unref (pixbuf);

  if (!gxr_overlay_set_alpha (overlay, 0.0f))
    return FALSE;

  return TRUE;
}

static void
gxr_overlay_model_finalize (GObject *gobject)
{
  GxrOverlayModel *self = GXR_OVERLAY_MODEL (gobject);
  GxrOverlayModelPrivate *priv = gxr_overlay_model_get_instance_private (self);
  g_object_unref (priv->overlay);
  G_OBJECT_CLASS (gxr_overlay_model_parent_class)->finalize (gobject);
}

/*
 * Sets render model to draw behind this overlay and the vertex color to use,
 * pass NULL for color to match the overlays vertex color
 */
gboolean
gxr_overlay_model_set_model (GxrOverlayModel *self, gchar *name,
                             graphene_vec4_t *color)
{
  GxrOverlayModelPrivate *priv = gxr_overlay_model_get_instance_private (self);
  return gxr_overlay_set_model (priv->overlay, name, color);
}

gboolean
gxr_overlay_model_get_model (GxrOverlayModel *self, gchar *name,
                             graphene_vec4_t *color, uint32_t *id)
{
  GxrOverlayModelPrivate *priv = gxr_overlay_model_get_instance_private (self);
  gxr_overlay_get_model (priv->overlay, name, color, id);
  return TRUE;
}

GxrOverlay*
gxr_overlay_model_get_overlay (GxrOverlayModel *self)
{
  GxrOverlayModelPrivate *priv = gxr_overlay_model_get_instance_private (self);
  return priv->overlay;
}
