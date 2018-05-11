#include <stdlib.h>
#include <math.h>
#include <cairo.h>
#include <clutter/clutter.h>

#include "clutter_content.h"

int
main (int argc, char *argv[])
{
  /* initialize Clutter */
  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    return EXIT_FAILURE;

  ClutterActor* stage = clutter_stage_new ();
  create_rotated_quad_stage (stage);

  clutter_actor_show (stage);

  /* quit on destroy */
  g_signal_connect (stage, "destroy", G_CALLBACK (clutter_main_quit), NULL);

  clutter_main ();

  return EXIT_SUCCESS;
}
