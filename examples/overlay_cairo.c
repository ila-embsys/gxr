/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <math.h>
#include <glib.h>
#include <GL/gl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

#include <openvr-glib.h>

#include "openvr-system.h"
#include "openvr-overlay.h"

#define WIDTH 1280
#define HEIGHT 720
#define STRIDE (WIDTH * 4)

#include "cairo_content.h"

void
draw_cairo (cairo_t *cr, unsigned width, unsigned height)
{
  draw_gradient_quad (cr, width, height);
  draw_gradient_circle (cr, width, height);
}

#define OFFSET(channels, stride, x, y) ((x) * channels + (gsize)(y) * stride)

cairo_surface_t *
cairo_surface_flip (cairo_surface_t *src)
{
  int width = cairo_image_surface_get_width (src);
  int height = cairo_image_surface_get_height (src);
  int stride = cairo_image_surface_get_stride (src);
  cairo_format_t format = cairo_image_surface_get_format (src);

  unsigned char *dest_pixels = g_malloc (stride*height * sizeof(unsigned char));

  unsigned n_channels = 3;
  switch (format)
  {
  case CAIRO_FORMAT_ARGB32:
    n_channels = 4;
    break;
  case CAIRO_FORMAT_RGB24:
  case CAIRO_FORMAT_RGB16_565:
  case CAIRO_FORMAT_RGB30:
    n_channels = 3;
    break;
  case CAIRO_FORMAT_A8:
    break;
  case CAIRO_FORMAT_A1:
    break;
  default:
    break;
  }

  const guint8 *src_pixels = cairo_image_surface_get_data (src);

  for (gint y = 0; y < height; y++)
    {
      const guchar *p = src_pixels + OFFSET (n_channels, stride, 0, y);
      guchar *q = dest_pixels + OFFSET (n_channels, stride, 0, height - y - 1);
      memcpy (q, p, stride);
    }

  cairo_surface_t *dest = cairo_image_surface_create_for_data (dest_pixels,
                                                               format,
                                                               width,
                                                               height,
                                                               stride);

  return dest;
}

cairo_surface_t*
create_cairo_surface (unsigned char *image)
{
  cairo_surface_t *surface =
    cairo_image_surface_create_for_data (image,
                                         CAIRO_FORMAT_ARGB32,
                                         WIDTH, HEIGHT,
                                         STRIDE);

  cairo_t *cr = cairo_create (surface);

  cairo_rectangle (cr, 0, 0, WIDTH, HEIGHT);
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_fill (cr);

  draw_cairo (cr, WIDTH, HEIGHT);

  cairo_destroy (cr);

  cairo_surface_t *flipped_surface = cairo_surface_flip (surface);

  g_object_unref (surface);

  return flipped_surface;
}

gboolean
timeout_callback (gpointer data)
{
  OpenVROverlay *overlay = (OpenVROverlay*) data;
  openvr_overlay_poll_event (overlay);
  return TRUE;
}

void
print_gl_context_info ()
{
  const GLubyte *version = glGetString (GL_VERSION);
  const GLubyte *vendor = glGetString (GL_VENDOR);
  const GLubyte *renderer = glGetString (GL_RENDERER);

  GLint major;
  GLint minor;
  glGetIntegerv (GL_MAJOR_VERSION, &major);
  glGetIntegerv (GL_MINOR_VERSION, &minor);

  g_print ("%s %s %s (%d.%d)\n", vendor, renderer, version, major, minor);
}

gboolean
init_offscreen_gl ()
{
  CoglError *error = NULL;
  CoglContext *ctx = cogl_context_new (NULL, &error);
  if (!ctx)
    {
      fprintf (stderr, "Failed to create context: %s\n", error->message);
      return FALSE;
    }

  // TODO: implement CoglOffscreen frameuffer
  CoglOnscreen *onscreen = cogl_onscreen_new (ctx, 800, 600);

  CoglDisplay *cogl_display;
  CoglRenderer *cogl_renderer;

  cogl_display = cogl_context_get_display (ctx);
  cogl_renderer = cogl_display_get_renderer (cogl_display);

  switch (cogl_renderer_get_driver (cogl_renderer))
    {
    case COGL_DRIVER_GL3:
      {
        g_print ("COGL_DRIVER_GL3.\n");
        break;
      }
    case COGL_DRIVER_GL:
      {
        g_print ("COGL_DRIVER_GL.\n");
        break;
      }
    default:
      g_print ("Other backend.\n");
      break;
    }


  return ctx != NULL && onscreen != NULL;
}

static void
_move_cb (OpenVROverlay  *overlay,
          GdkEventMotion *event,
          gpointer        data)
{
  // g_print ("move: %f %f (%d)\n", event->x, event->y, event->time);
}

static void
_press_cb (OpenVROverlay  *overlay,
           GdkEventButton *event,
           gpointer        data)
{
  g_print ("press: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}

static void
_release_cb (OpenVROverlay  *overlay,
             GdkEventButton *event,
             gpointer        data)
{
  g_print ("release: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
}

static void
_show_cb (OpenVROverlay *overlay,
          gpointer       data)
{
  g_print ("show\n");

  /* skip rendering if the overlay isn't available or visible */
  gboolean is_unavailable = !openvr_overlay_is_valid (overlay) ||
                            !openvr_overlay_is_available (overlay);
  gboolean is_invisible = !openvr_overlay_is_visible (overlay) &&
                          !openvr_overlay_thumbnail_is_visible (overlay);

  if (is_unavailable || is_invisible)
    return;

  cairo_surface_t* surface = (cairo_surface_t*) data;
  openvr_overlay_upload_cairo_surface (overlay, surface);
}

static void
_destroy_cb (OpenVROverlay *overlay,
             gpointer       data)
{
  g_print ("destroy\n");
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}

int
test_cat_overlay ()
{
  GMainLoop *loop;

  unsigned char image[STRIDE*HEIGHT];
  cairo_surface_t* surface = create_cairo_surface (image);

  if (surface == NULL) {
    fprintf (stderr, "Could not create cairo surface.\n");
    return -1;
  }

  loop = g_main_loop_new (NULL, FALSE);

  if (!init_offscreen_gl ())
  {
    fprintf (stderr, "Error: Could not create GL context.\n");
    return FALSE;
  }

  print_gl_context_info ();

  OpenVRSystem * system = openvr_system_new ();
  gboolean ret = openvr_system_init_overlay (system);

  g_assert (ret);
  g_assert (openvr_system_is_available (system));
  g_assert (openvr_system_is_installed ());

  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create (overlay, "examples.cairo", "Gradient");

  if (!openvr_overlay_is_valid (overlay) ||
      !openvr_overlay_is_available (overlay))
  {
    fprintf (stderr, "Overlay unavailable.\n");
    return -1;
  }

  openvr_overlay_set_mouse_scale (overlay, (float) WIDTH, (float) HEIGHT);

  g_signal_connect (overlay, "motion-notify-event", (GCallback) _move_cb, NULL);
  g_signal_connect (overlay, "button-press-event", (GCallback) _press_cb, loop);
  g_signal_connect (
    overlay, "button-release-event", (GCallback) _release_cb, NULL);
  g_signal_connect (overlay, "show", (GCallback) _show_cb, surface);
  g_signal_connect (overlay, "destroy", (GCallback) _destroy_cb, loop);

  g_timeout_add (20, timeout_callback, overlay);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_object_unref (overlay);
  cairo_surface_destroy (surface);

  return 0;
}

int main (int argc, char *argv[]) {
  return test_cat_overlay ();
}
