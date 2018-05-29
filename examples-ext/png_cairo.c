/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <math.h>
#include <cairo.h>

#include "cairo_content.h"

#define WIDTH 1280
#define HEIGHT 720
#define STRIDE (WIDTH * 4)

void
draw (cairo_t *cr, unsigned width, unsigned height)
{
  draw_gradient_quad (cr, width, height);
  draw_gradient_circle (cr, width, height);
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
