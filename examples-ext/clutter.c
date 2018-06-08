#include <stdlib.h>
#include <math.h>
#include <cairo.h>

#define COGL_ENABLE_EXPERIMENTAL_API 1
#define CLUTTER_ENABLE_EXPERIMENTAL_API 1

#include <cogl/cogl.h>

#include <clutter/clutter.h>

#include "clutter_content.h"



int
main (int argc, char *argv[])
{
  /* initialize Clutter */
  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    return EXIT_FAILURE;

  ClutterActor * stage = clutter_stage_new ();
  create_rotated_quad_stage (stage);

  clutter_actor_show (stage);

  /* quit on destroy */
  g_signal_connect (stage, "destroy", G_CALLBACK (clutter_main_quit), NULL);

  ClutterBackend *backend = clutter_get_default_backend ();
  CoglContext *ctx = clutter_backend_get_cogl_context (backend);

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

  clutter_main ();

  return EXIT_SUCCESS;
}
