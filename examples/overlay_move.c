/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <graphene.h>
#include <signal.h>
#include <glib/gprintf.h>

#include <openvr-glib.h>

#include "openvr-context.h"
#include "openvr-compositor.h"
#include "openvr-controller.h"
#include "openvr-math.h"
#include "openvr-overlay.h"
#include "openvr-vulkan-uploader.h"

#include "cairo_content.h"

OpenVRVulkanTexture *texture;

GSList *controllers = NULL;

GSList *cat_overlays = NULL;

OpenVROverlay *pointer;
OpenVROverlay *intersection;

OpenVROverlay *current_hover_overlay = NULL;
OpenVROverlay *current_grab_overlay = NULL;
graphene_matrix_t *current_grab_matrix = NULL;
graphene_matrix_t *current_hover_matrix = NULL;

OpenVRVulkanTexture *cairo_texture;

OpenVROverlay *control_overlay = NULL;

GMainLoop *loop;

GHashTable *initial_overlay_transforms = NULL;

float distance = 1.f;

void
_sigint_cb (int sig_num)
{
  if (sig_num == SIGINT)
    g_main_loop_quit (loop);
}

static void
_controller_poll (gpointer controller, gpointer unused)
{
  (void) unused;
  openvr_controller_poll_event ((OpenVRController*) controller);
}

static void
_register_controller_events (gpointer controller, gpointer unused);

gboolean
_overlay_event_cb (gpointer unused)
{
  (void) unused;
  // TODO: Controllers should be registered in the system event callback
  if (controllers == NULL)
    {
      controllers = openvr_controller_enumerate ();
      g_slist_foreach (controllers, _register_controller_events, NULL);
    }

  g_slist_foreach (controllers, _controller_poll, NULL);

  // openvr_overlay_poll_event (overlay);
  return TRUE;
}

GdkPixbuf *
load_gdk_pixbuf (const gchar* name)
{
  GError * error = NULL;
  GdkPixbuf *pixbuf_rgb = gdk_pixbuf_new_from_resource (name, &error);

  if (error != NULL)
    {
      fprintf (stderr, "Unable to read file: %s\n", error->message);
      g_error_free (error);
      return NULL;
    }

  GdkPixbuf *pixbuf = gdk_pixbuf_add_alpha (pixbuf_rgb, false, 0, 0, 0);
  g_object_unref (pixbuf_rgb);
  return pixbuf;
}


void
_update_intersection_position (OpenVROverlay *overlay,
                               OpenVRIntersectionEvent *event)
{
  graphene_matrix_t transform;
  graphene_matrix_init_from_matrix (&transform, &event->transform);
  openvr_math_matrix_set_translation (&transform, &event->intersection_point);
  openvr_overlay_set_transform_absolute (overlay, &transform);
  openvr_overlay_show (overlay);
}

void
_test_overlay_intersection (OpenVRMotion3DEvent *event)
{
  GSList *l;
  gboolean found_intersection = FALSE;
  for (l = cat_overlays; l != NULL; l = l->next)
    {
      OpenVROverlay *overlay = (OpenVROverlay*) l->data;

      graphene_point3d_t intersection_point;
      if (openvr_overlay_test_intersection (overlay, &event->transform,
                                            &intersection_point))
        {
          found_intersection = TRUE;

          if (current_hover_overlay != NULL)
            {
              graphene_vec3_t unmarked_color;
              graphene_vec3_init (&unmarked_color, 1.f, 1.f, 1.f);
              openvr_overlay_set_color (current_hover_overlay, &unmarked_color);
            }

          current_hover_overlay = overlay;

          graphene_matrix_init_from_matrix (current_hover_matrix,
                                            &event->transform);

          graphene_vec3_t marked_color;
          graphene_vec3_init (&marked_color, .8f, .2f, .2f);
          openvr_overlay_set_color (overlay, &marked_color);

          /* Distance */
          graphene_vec3_t intersection_vec;
          graphene_vec3_init (&intersection_vec,
                               intersection_point.x,
                               intersection_point.y,
                               intersection_point.z);

          graphene_vec3_t controller_position;
          openvr_math_vec3_init_from_matrix (&controller_position,
                                             &event->transform);

          graphene_vec3_t distance_vec;
          graphene_vec3_subtract (&controller_position,
                                  &intersection_vec,
                                  &distance_vec);

          distance = graphene_vec3_length (&distance_vec);
          // g_print ("distance %.2f\n", distance);
        }
    }

  /* Test control overlays */
  if (!found_intersection)
    {
      graphene_point3d_t intersection_point;
      if (openvr_overlay_test_intersection (control_overlay, &event->transform,
                                            &intersection_point))
        {
          found_intersection = TRUE;

          if (current_hover_overlay != NULL)
            {
              graphene_vec3_t unmarked_color;
              graphene_vec3_init (&unmarked_color, 1.f, 1.f, 1.f);
              openvr_overlay_set_color (current_hover_overlay, &unmarked_color);
            }

          current_hover_overlay = control_overlay;

          graphene_vec3_t marked_color;
          graphene_vec3_init (&marked_color, .2f, .2f, .8f);
          openvr_overlay_set_color (control_overlay, &marked_color);
        }
    }

  if (!found_intersection)
    {
      if (current_hover_overlay != NULL)
        {
          graphene_vec3_t unmarked_color;
          graphene_vec3_init (&unmarked_color, 1.f, 1.f, 1.f);
          openvr_overlay_set_color (current_hover_overlay, &unmarked_color);
          current_hover_overlay = NULL;
        }

      openvr_overlay_hide (intersection);
    }
}

static void
_motion_3d_cb (OpenVRController    *controller,
               OpenVRMotion3DEvent *event,
               gpointer             data)
{
  (void) controller;
  (void) data;

  /* Transform Pointer */
  {
    graphene_matrix_t scale_matrix;
    graphene_matrix_init_scale (&scale_matrix, 1.0f, 1.0f, 5.0f);

    graphene_matrix_t scaled;
    graphene_matrix_multiply (&scale_matrix, &event->transform, &scaled);

    openvr_overlay_set_transform_absolute (pointer, &scaled);
  }

  /* Drag */
  if (current_grab_overlay != NULL)
    {
      graphene_point3d_t translation_point;
      graphene_point3d_init (&translation_point, .0f, .0f, -distance);

      graphene_matrix_t transformation_matrix;
      graphene_matrix_init_translate (&transformation_matrix, &translation_point);

      graphene_matrix_t transformed;
      graphene_matrix_multiply (&transformation_matrix,
                                &event->transform,
                                &transformed);

      openvr_overlay_set_transform_absolute (current_grab_overlay, &transformed);
    }
  else
    {
      _test_overlay_intersection (event);
    }

  free (event);
}

static void
_intersection_cb (OpenVROverlay           *overlay,
                  OpenVRIntersectionEvent *event,
                  gpointer                 data)
{
  (void) overlay;
  (void) data;

  // if we have an intersection point, move the pointer overlay there
  if (event->has_intersection)
    _update_intersection_position (intersection, event);
  else
    openvr_overlay_hide (intersection);

  // TODO: because this is a custom event with a struct that has been allocated
  // specifically for us, we need to free it. Maybe reuse?
  free (event);
}

static void
_reset_cat_overlays ()
{
  GSList *l;
  for (l = cat_overlays; l != NULL; l = l->next)
    {
      OpenVROverlay *overlay = (OpenVROverlay*) l->data;
      graphene_matrix_t *transform =
        g_hash_table_lookup (initial_overlay_transforms, overlay);
      openvr_overlay_set_transform_absolute (overlay, transform);
    }
}

static void
_press_cb (OpenVRController *controller, GdkEventButton *event, gpointer data)
{
  (void) controller;
  (void) data;
  g_print ("press: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);

  if (event->button == OPENVR_BUTTON_TRIGGER)
    {
      if (current_hover_overlay != NULL)
        {
          if (current_hover_overlay == control_overlay)
            {
              g_print ("Control clicked.\n");
              _reset_cat_overlays ();
            }
          else
            {
              current_grab_overlay = current_hover_overlay;

              graphene_matrix_init_from_matrix (current_grab_matrix,
                                                current_hover_matrix);

              openvr_overlay_hide (intersection);

              graphene_vec3_t marked_color;
              graphene_vec3_init (&marked_color, .2f, .8f, .2f);
              openvr_overlay_set_color (current_grab_overlay, &marked_color);
            }
        }
    }

}

static void
_release_cb (OpenVRController *controller, GdkEventButton *event, gpointer data)
{
  (void) controller;
  (void) data;
  g_print ("release: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);

  if (event->button == OPENVR_BUTTON_TRIGGER)
    {
      if (current_grab_overlay != NULL)
        {
          graphene_vec3_t unmarked_color;
          graphene_vec3_init (&unmarked_color, 1.f, 1.f, 1.f);
          openvr_overlay_set_color (current_grab_overlay, &unmarked_color);
        }
      current_grab_overlay = NULL;
    }

}

GdkPixbuf *
_create_empty_pixbuf (uint32_t width, uint32_t height)
{
  guchar pixels[height * width * 4];
  memset (pixels, 0, height * width * 4 * sizeof (guchar));
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB,
                                                TRUE, 8, width, height,
                                                4 * width, NULL, NULL);
  return pixbuf;
}

OpenVROverlay *
_init_pointer_overlay ()
{
  OpenVROverlay *overlay = openvr_overlay_new ();

  openvr_overlay_create_width (overlay, "pointer", "Pointer", 0.01f);

  if (!openvr_overlay_is_valid (overlay))
    {
      g_printerr ("Overlay unavailable.\n");
      return NULL;
    }

  // TODO: wrap the uint32 of the sort order in some sort of hierarchy
  // for now: The pointer itself should *always* be visible on top of overlays,
  // so use the max value here
  openvr_overlay_set_sort_order (overlay, UINT32_MAX);

  GdkPixbuf *pixbuf = _create_empty_pixbuf (10, 10);
  if (pixbuf == NULL)
    return NULL;

  openvr_overlay_set_gdk_pixbuf_raw (overlay, pixbuf);
  g_object_unref (pixbuf);

  struct HmdColor_t color = {
    .r = 1.0f,
    .g = 1.0f,
    .b = 1.0f,
    .a = 1.0f
  };

  if (!openvr_overlay_set_model (overlay, "{system}laser_pointer", &color))
    return NULL;

  char name_ret[k_unMaxPropertyStringSize];
  struct HmdColor_t color_ret = {};

  uint32_t id;
  if (!openvr_overlay_get_model (overlay, name_ret, &color_ret, &id))
    return NULL;

  g_print ("GetOverlayRenderModel returned id %d\n", id);
  g_print ("GetOverlayRenderModel name %s\n", name_ret);

  if (!openvr_overlay_set_width_meters (overlay, 0.01f))
    return NULL;

  if (!openvr_overlay_set_alpha (overlay, 0.0f))
    return NULL;

  if (!openvr_overlay_show (overlay))
    return NULL;

  return overlay;
}

bool
_init_openvr ()
{
  if (!openvr_context_is_installed ())
    {
      g_printerr ("VR Runtime not installed.\n");
      return false;
    }

  OpenVRContext *context = openvr_context_get_instance ();
  if (!openvr_context_init_overlay (context))
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  if (!openvr_context_is_valid (context))
    {
      g_printerr ("Could not load OpenVR function pointers.\n");
      return false;
    }

  return true;
}

gboolean
_init_cat_overlays (OpenVRVulkanUploader *uploader)
{
  GdkPixbuf *pixbuf = load_gdk_pixbuf ("/res/cat.jpg");
  if (pixbuf == NULL)
    return -1;

  texture = openvr_vulkan_texture_new ();
  openvr_vulkan_client_load_pixbuf (OPENVR_VULKAN_CLIENT (uploader), texture,
                                    pixbuf);

  float width = .5f;

  float pixbuf_aspect = (float) gdk_pixbuf_get_width (pixbuf) /
                        (float) gdk_pixbuf_get_height (pixbuf);

  float height = width / pixbuf_aspect;

  g_print ("pixbuf: %.2f x %.2f\n", (float) gdk_pixbuf_get_width (pixbuf),
                                    (float) gdk_pixbuf_get_height (pixbuf));
  g_print ("meters: %.2f x %.2f\n", width, height);


  uint32_t i = 0;

  initial_overlay_transforms = g_hash_table_new (g_direct_hash, g_direct_equal);

  for (float x = 0; x < 5 * width; x += width)
    for (float y = 0; y < 5 * height; y += height)
      {
        OpenVROverlay *cat = openvr_overlay_new ();

        char overlay_name[10];
        g_sprintf (overlay_name, "cat%d", i);

        openvr_overlay_create_width (cat, overlay_name, "A Cat", width);

        if (!openvr_overlay_is_valid (cat))
          {
            fprintf (stderr, "Overlay unavailable.\n");
            return -1;
          }

        graphene_point3d_t position = {
          .x = x,
          .y = y,
          .z = -3
        };

        graphene_matrix_t *transform = graphene_matrix_alloc ();
        graphene_matrix_init_translate (transform, &position);

        g_hash_table_insert (initial_overlay_transforms, cat, transform);

        openvr_overlay_set_transform_absolute (cat, transform);

        openvr_overlay_set_mouse_scale (cat,
                                        (float) gdk_pixbuf_get_width (pixbuf),
                                        (float) gdk_pixbuf_get_height (pixbuf));

        openvr_vulkan_uploader_submit_frame (uploader, cat, texture);

        cat_overlays = g_slist_append (cat_overlays, cat);

        if (!openvr_overlay_show (cat))
          return -1;

        g_signal_connect (cat, "intersection-event",
                          (GCallback) _intersection_cb, NULL);

        i++;
      }

  g_object_unref (pixbuf);

  return TRUE;
}

gboolean
_init_intersection_overlay (OpenVRVulkanUploader *uploader)
{
  GdkPixbuf *pixbuf = load_gdk_pixbuf ("/res/crosshair.png");
  if (pixbuf == NULL)
    return FALSE;

  OpenVRVulkanTexture *intersection_texture = openvr_vulkan_texture_new ();
  openvr_vulkan_client_load_pixbuf (OPENVR_VULKAN_CLIENT (uploader),
                                    intersection_texture, pixbuf);
  g_object_unref (pixbuf);

  intersection = openvr_overlay_new ();
  openvr_overlay_create_width (intersection, "pointer.intersection",
                               "Interection", 0.15);

  if (!openvr_overlay_is_valid (intersection))
    {
      g_printerr ("Overlay unavailable.\n");
      return FALSE;
    }

  // for now: The crosshair should always be visible, except the pointer can
  // occlude it. The pointer has max sort order, so the crosshair gets max -1
  openvr_overlay_set_sort_order (intersection, UINT32_MAX - 1);

  openvr_vulkan_uploader_submit_frame (uploader,
                                       intersection, intersection_texture);

  return TRUE;
}

void
_draw_cairo (cairo_t *cr, unsigned width, unsigned height, const gchar* text)
{
  double r0;
  if (width < height)
    r0 = (double) width / 3.0;
  else
    r0 = (double) height / 3.0;

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
  cairo_pattern_add_color_stop_rgba (pat, 0, .3, .3, .3, 1);
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
  cairo_text_extents (cr, text, &extents);

  cairo_move_to (cr,
                 center_x - extents.width / 2,
                 center_y  - extents.height / 2);
  cairo_set_source_rgb (cr, 0.9, 0.9, 0.9);
  cairo_show_text (cr, text);
}

cairo_surface_t*
_create_cairo_surface (unsigned char *image, uint32_t width,
                       uint32_t height, const gchar *text)
{
  cairo_surface_t *surface =
    cairo_image_surface_create_for_data (image,
                                         CAIRO_FORMAT_ARGB32,
                                         width, height,
                                         width * 4);

  cairo_t *cr = cairo_create (surface);

  cairo_rectangle (cr, 0, 0, width, height);
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_fill (cr);

  _draw_cairo (cr, width, height, text);

  cairo_destroy (cr);

  return surface;
}

gboolean
_init_control_overlays (OpenVRVulkanUploader *uploader)
{
  guint width = 200;
  guint height = 200;
  unsigned char image[4 * width * height];
  cairo_surface_t* surface = _create_cairo_surface (image, width,
                                                    height, "Reset");

  if (surface == NULL) {
    fprintf (stderr, "Could not create cairo surface.\n");
    return -1;
  }

  if (!openvr_vulkan_uploader_init_vulkan (uploader, true))
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return false;
  }

  cairo_texture = openvr_vulkan_texture_new ();

  openvr_vulkan_client_load_cairo_surface (OPENVR_VULKAN_CLIENT (uploader),
                                           cairo_texture, surface);

  /* create openvr overlay */
  control_overlay = openvr_overlay_new ();
  openvr_overlay_create_width (control_overlay, "cairo.control",
                               "Cairo Control", 0.5f);

  if (!openvr_overlay_is_valid (control_overlay))
  {
    g_printerr ("Overlay unavailable.\n");
    return -1;
  }

  graphene_point3d_t position = {
    .x =  0.0f,
    .y =  0.0f,
    .z = -1.0f
  };

  graphene_matrix_t transform;
  graphene_matrix_init_translate (&transform, &position);
  openvr_overlay_set_transform_absolute (control_overlay, &transform);

  // openvr_overlay_set_mouse_scale (overlay, (float) WIDTH, (float) HEIGHT);

  if (!openvr_overlay_show (control_overlay))
    return -1;

  openvr_vulkan_uploader_submit_frame (uploader,
                                       control_overlay, cairo_texture);

  return TRUE;
}

static void
_register_controller_events (gpointer controller, gpointer unused)
{
  (void) unused;
  OpenVRController* c = (OpenVRController*) controller;
  g_signal_connect (c, "motion-3d-event", (GCallback) _motion_3d_cb, NULL);
  g_signal_connect (c, "button-press-event", (GCallback) _press_cb, NULL);
  g_signal_connect (c, "button-release-event", (GCallback) _release_cb, NULL);
}

int
main ()
{
  loop = g_main_loop_new (NULL, FALSE);

  current_grab_matrix = graphene_matrix_alloc ();
  current_hover_matrix = graphene_matrix_alloc ();

  /* init openvr */
  if (!_init_openvr ())
    return -1;

  OpenVRVulkanUploader *uploader = openvr_vulkan_uploader_new ();
  if (!openvr_vulkan_uploader_init_vulkan (uploader, true))
    {
      g_printerr ("Unable to initialize Vulkan!\n");
      return false;
    }

  pointer = _init_pointer_overlay ();

  if (!_init_intersection_overlay (uploader))
    return -1;

  if (!_init_cat_overlays (uploader))
    return -1;

  if (!_init_control_overlays (uploader))
    return -1;

  g_timeout_add (20, _overlay_event_cb, NULL);

  signal (SIGINT, _sigint_cb);

  /* start glib main loop */
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_print ("bye\n");

  g_object_unref (pointer);
  g_object_unref (intersection);
  g_object_unref (texture);
  g_object_unref (uploader);

  g_slist_free_full (controllers, g_object_unref);
  g_slist_free_full (cat_overlays, g_object_unref);

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  return 0;
}
