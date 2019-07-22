/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <math.h>
#include <glib.h>
#include <GL/gl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

#include "gxr.h"

#define MAXWIDTH 1920
#define MAXHEIGHT 1080
#define MAXSTRIDE MAXWIDTH * 4

#include "cairo_content.h"

#define OVERLAY_NUM 10

typedef struct Example
{
  GMainLoop *loop;
  GulkanClient *uploader;
  OpenVROverlay *overlays[OVERLAY_NUM];
  GulkanTexture *textures[OVERLAY_NUM];
  int tex_count;
} Example;

typedef struct ExampleOverlayNum
{
  Example *example;
  int i;
} ExampleOverlayNum;

static cairo_surface_t*
create_cairo_surface (unsigned char *image, char *text)
{

  int WIDTH =  MAXWIDTH;
  int HEIGHT = MAXHEIGHT;
  /* To test with randomized texture dimensions
  WIDTH -= rand () % 50;
  HEIGHT -= rand () % 50;
   */
  int STRIDE = (WIDTH * 4);
  cairo_surface_t *surface =
    cairo_image_surface_create_for_data (image,
                                         CAIRO_FORMAT_ARGB32,
                                         WIDTH, HEIGHT,
                                         STRIDE);

  cairo_t *cr = cairo_create (surface);

  cairo_rectangle (cr, 0, 0, WIDTH, HEIGHT);
  cairo_set_source_rgb (cr, 0.2, 0, 0);
  cairo_fill (cr);

  cairo_select_font_face (cr, "cairo :monospace",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, 72);

  cairo_move_to (cr, 50, 50);
  cairo_set_source_rgb (cr, 0.9, 0.9, 0.9);
  cairo_show_text (cr, text);

  cairo_destroy (cr);

  return surface;
}

static unsigned char image[MAXSTRIDE*MAXHEIGHT];
static gboolean
timeout_callback (gpointer data)
{
  ExampleOverlayNum *en = (ExampleOverlayNum*) data;
  Example *self = en->example;
  OpenVROverlay *overlay = self->overlays[en->i];

  char key[k_unVROverlayMaxKeyLength];
  snprintf (key, k_unVROverlayMaxKeyLength, "texture #%d\n", self->tex_count++);

  cairo_surface_t *surface = create_cairo_surface (image, key);

  if (surface == NULL) {
    fprintf (stderr, "Could not create cairo surface.\n");
    return -1;
  }

  GulkanTexture *n =
    gulkan_client_texture_new_from_cairo_surface (self->uploader,
                                                  surface,
                                                  VK_FORMAT_R8G8B8A8_UNORM,
                                                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  openvr_overlay_submit_texture (overlay, self->uploader, n);

  if (self->textures[en->i] != NULL)
    {
      g_object_unref (self->textures[en->i]);
      self->textures[en->i] = NULL;
    }
  self->textures[en->i] = n;

  cairo_surface_destroy (surface);

  openvr_overlay_poll_event (overlay);
  return TRUE;
}

static void
_press_cb (OpenVROverlay  *overlay,
           GdkEventButton *event,
           gpointer        data)
{
  (void) overlay;
  Example *self = (Example*) data;
  g_print ("press: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
  GMainLoop *loop = self->loop;
  g_main_loop_quit (loop);
}

static void
_destroy_cb (OpenVROverlay *overlay,
             gpointer       data)
{
  (void) overlay;
  Example *self = (Example*) data;
  g_print ("destroy\n");
  g_main_loop_quit (self->loop);
}

static bool
_init_openvr ()
{
  OpenVRContext *context = openvr_context_get_instance ();
  if (!openvr_context_initialize (context, OPENVR_APP_OVERLAY))
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  return true;
}

int main () {
  Example ex = {
    .loop = g_main_loop_new (NULL, FALSE),
    .tex_count = 0
  };

  /* init openvr */
  if (!_init_openvr ())
    return -1;

  /* Upload vulkan texture */
  ex.uploader = openvr_compositor_gulkan_client_new ();
  if (!ex.uploader)
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return false;
  }

  ExampleOverlayNum nums[OVERLAY_NUM];
  for (int i = 0; i < OVERLAY_NUM; i++)
    {
      nums[i].example = &ex;
      nums[i].i = i;
    }

  /* create openvr overlay */
  float x_offset = 0;
  for (int i = 0; i < OVERLAY_NUM; i++)
    {
      ex.textures[i] = NULL;
      ex.overlays[i] = openvr_overlay_new ();
      char key[k_unVROverlayMaxKeyLength];
      snprintf (key, k_unVROverlayMaxKeyLength, "examples.cairo-%d\n", i);
      openvr_overlay_create (ex.overlays[i], key, "Gradient");

      if (!openvr_overlay_is_valid (ex.overlays[i]))
      {
        fprintf (stderr, "Overlay unavailable.\n");
        return -1;
      }

      float diff = 0;
      if (i > 0)
         openvr_overlay_get_width_meters (ex.overlays[i-1], &diff);
      x_offset += diff;

      g_print ("Offset %f\n", x_offset);

      graphene_point3d_t p = { .x = -2 + x_offset, .y = 0.5, .z = -2 };
      openvr_overlay_set_translation (ex.overlays[i], &p);

      if (!openvr_overlay_show (ex.overlays[i]))
        return -1;

      g_signal_connect (ex.overlays[i], "button-press-event", (GCallback) _press_cb, &ex);
      g_signal_connect (ex.overlays[i], "destroy", (GCallback) _destroy_cb, &ex);

      g_timeout_add (75, timeout_callback, &nums[i]);
    }

  g_main_loop_run (ex.loop);
  g_main_loop_unref (ex.loop);

  for (int i = 0; i < OVERLAY_NUM; i++)
    {
      g_object_unref (ex.overlays[i]);
      g_object_unref (ex.textures[i]);
    }

  g_object_unref (ex.uploader);

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  return 0;
}
