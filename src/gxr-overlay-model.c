/*
 * gxresktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-overlay-model.h"

struct  _GxrOverlayModel
{
  GObject parent;
  GxrOverlay *overlay;
};

G_DEFINE_TYPE (GxrOverlayModel, gxr_overlay_model, G_TYPE_OBJECT)

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
  (void) self;
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
gxr_overlay_model_new (GxrContext *context, gchar* key)
{
  GxrOverlayModel *self =
    (GxrOverlayModel*) g_object_new (GXR_TYPE_OVERLAY_MODEL, 0);

  self->overlay = gxr_overlay_new (context, key);
  if (!gxr_overlay_is_valid (self->overlay))
    {
      g_printerr ("Model overlay %s unavailable.\n", key);
      return NULL;
    }

  GdkPixbuf *pixbuf = _create_empty_pixbuf (10, 10);
  if (pixbuf == NULL)
    return NULL;

  /*
   * Overlay needs a texture to be set to show model
   * See https://github.com/ValveSoftware/openvr/issues/496
   */
  gxr_overlay_set_gdk_pixbuf_raw (self->overlay, pixbuf);
  g_object_unref (pixbuf);

  if (!gxr_overlay_set_alpha (self->overlay, 0.0f))
    return NULL;

  return self;
}

static void
gxr_overlay_model_finalize (GObject *gobject)
{
  GxrOverlayModel *self = GXR_OVERLAY_MODEL (gobject);
  g_object_unref (self->overlay);
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
  return gxr_overlay_set_model (self->overlay, name, color);
}

gboolean
gxr_overlay_model_get_model (GxrOverlayModel *self, gchar *name,
                             graphene_vec4_t *color, uint32_t *id)
{
  gxr_overlay_get_model (self->overlay, name, color, id);
  return TRUE;
}

GxrOverlay*
gxr_overlay_model_get_overlay (GxrOverlayModel *self)
{
  return self->overlay;
}
