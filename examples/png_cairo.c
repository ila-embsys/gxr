/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <math.h>
#include <cairo.h>

#define WIDTH 1280
#define HEIGHT 720
#define STRIDE (WIDTH * 4)

void
draw (cairo_t *cr, unsigned width, unsigned height)
{
  cairo_pattern_t *pat;
  pat = cairo_pattern_create_linear (0.0, 0.0,  0.0, height);
  cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0, 0, 1);
  cairo_pattern_add_color_stop_rgba (pat, 0, 1, 1, 1, 1);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  double center_x = (double) width / 2.0;
  double center_y = (double) height / 2.0;

  double r0;
  if (width < height)
    r0 = (double) width / 10.0;
  else
    r0 = (double) height / 10.0;

  double radius = r0 * 3.0;
  double r1 = r0 * 5.0;

  double cx0 = center_x - r0 / 2.0;
  double cy0 = center_y - r0;
  double cx1 = center_x - r0;
  double cy1 = center_y - r0;

  pat = cairo_pattern_create_radial (cx0, cy0, r0,
                                     cx1, cy1, r1);
  cairo_pattern_add_color_stop_rgba (pat, 0, 1, 1, 1, 1);
  cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0, 0, 1);
  cairo_set_source (cr, pat);
  cairo_arc (cr, center_x, center_y, radius, 0, 2 * M_PI);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  cairo_surface_t * surface = cairo_get_target (cr);
  unsigned char * raw = cairo_image_surface_get_data (surface);
}

int
main (int argc, char **argv)
{
  unsigned char image[STRIDE*HEIGHT];

  cairo_surface_t *surface =
    cairo_image_surface_create_for_data (image,
                                         CAIRO_FORMAT_ARGB32,
                                         WIDTH, HEIGHT,
                                         STRIDE);

  cairo_t *cr = cairo_create (surface);

  cairo_rectangle (cr, 0, 0, WIDTH, HEIGHT);
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_fill (cr);

  draw (cr, WIDTH, HEIGHT);

  cairo_surface_write_to_png (surface, "cairo.png");

  cairo_destroy (cr);

  cairo_surface_destroy (surface);

  return 0;
}
