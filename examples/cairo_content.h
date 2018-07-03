#pragma once

#include <glib.h>

void
draw_gradient_quad (cairo_t *cr, unsigned width, unsigned height)
{
  cairo_pattern_t *pat = cairo_pattern_create_linear (0.0, 0.0, 0.0, height);
  cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0, 0, 1);
  cairo_pattern_add_color_stop_rgba (pat, 0, 1, 1, 1, 1);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);
}

void
draw_rotated_quad (cairo_t *cr, double width, double rotation)
{
  cairo_translate(cr, width/2, width/2);
  cairo_rotate(cr, rotation);

  cairo_rectangle(cr, -(width/4.0), -(width/4.0), width/2.0, width/2.0);
  cairo_set_source_rgb(cr, 0.9, 0.3, 0.3);
  cairo_fill(cr);
}

void
draw_fps (cairo_t     *cr,
          double       width,
          const gchar *str)
{
  cairo_identity_matrix (cr);
  cairo_select_font_face (cr, "Sans",
                          CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 24);

  cairo_text_extents_t extents;
  cairo_text_extents (cr, str, &extents);

  cairo_move_to (cr, width/2 - extents.width/2, width/2 - extents.height/2);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_show_text (cr, str);
}

void
draw_gradient_circle (cairo_t *cr, unsigned width, unsigned height)
{
  double r0;
  if (width < height)
    r0 = (double) width / 10.0;
  else
    r0 = (double) height / 10.0;

  double radius = r0 * 3.0;
  double r1 = r0 * 5.0;

  double center_x = (double) width / 2.0;
  double center_y = (double) height / 2.0;

  double cx0 = center_x - r0 / 2.0;
  double cy0 = center_y - r0;
  double cx1 = center_x - r0;
  double cy1 = center_y - r0;

  cairo_pattern_t *pat = cairo_pattern_create_radial (cx0, cy0, r0,
                                                      cx1, cy1, r1);
  cairo_pattern_add_color_stop_rgba (pat, 0, 1, 1, 1, 1);
  cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0, 0, 1);
  cairo_set_source (cr, pat);
  cairo_arc (cr, center_x, center_y, radius, 0, 2 * M_PI);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  cairo_select_font_face (cr, "Sans",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size (cr, 52.0);

  cairo_text_extents_t extents;
  cairo_text_extents (cr, "R", &extents);

  cairo_move_to (cr, center_x, center_y);
  cairo_set_source_rgb (cr, 0.8, 0.3, 0.3);
  cairo_show_text (cr, "R");

  double x_offset = extents.width;

  cairo_move_to (cr, center_x + x_offset, center_y);
  cairo_set_source_rgb (cr, 0.3, 0.8, 0.3);
  cairo_show_text (cr, "G");

  cairo_text_extents (cr, "G", &extents);
  x_offset += extents.width;

  cairo_move_to (cr, center_x + x_offset, center_y);
  cairo_set_source_rgb (cr, 0.3, 0.3, 0.8);
  cairo_show_text (cr, "B");
}

void
draw_rounded_quad (cairo_t *cr, unsigned width, unsigned height)
{
  /* rounded rectangle taken from:
   *
   *   http://cairographics.org/samples/rounded_rectangle/
   *
   * we leave 1 pixel around the edges to avoid jagged edges
   * when rotating the actor
   */
  double x             = 1.0,        /* parameters like cairo_rectangle */
         y             = 1.0,
         width_t       = width - 2.0,
         height_t      = height - 2.0,
         aspect        = 1.0,     /* aspect ratio */
         corner_radius = height_t / 20.0;   /* and corner curvature radius */

  double radius = corner_radius / aspect;
  double degrees = M_PI / 180.0;

  cairo_save (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_restore (cr);

  cairo_new_sub_path (cr);
  cairo_arc (cr, x + width_t - radius, y + radius,
             radius, -90 * degrees, 0 * degrees);
  cairo_arc (cr, x + width_t - radius, y + height_t - radius,
             radius, 0 * degrees, 90 * degrees);
  cairo_arc (cr, x + radius, y + height_t - radius,
             radius, 90 * degrees, 180 * degrees);
  cairo_arc (cr, x + radius, y + radius,
             radius, 180 * degrees, 270 * degrees);
  cairo_close_path (cr);

  cairo_set_source_rgba (cr, 0.5, 0.5, 1, 0.95);
  cairo_fill (cr);
}
