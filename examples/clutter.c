#include <stdlib.h>
#include <math.h>
#include <cairo.h>
#include <clutter/clutter.h>

#include "clutter_content.h"


static gboolean
repaint_cb (gpointer user_data)
{
  //g_print ("CLUTTER_REPAINT_FLAGS_POST_PAINT\n");
  ClutterStage* stage = CLUTTER_STAGE (user_data);
  guchar* pixels = clutter_stage_read_pixels (stage, 0, 0, 500, 500);

  // struct wl_surface * sf = clutter_wayland_stage_get_wl_surface (stage);

  return TRUE;
}

int
main (int argc, char *argv[])
{
  /* initialize Clutter */
  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    return EXIT_FAILURE;

  ClutterActor * stage = clutter_stage_new ();
  create_rotated_quad_stage (stage);

  //clutter_actor_set_offscreen_redirect (stage,
  //                                      CLUTTER_OFFSCREEN_REDIRECT_ALWAYS);

  clutter_actor_show (stage);

  clutter_threads_add_repaint_func_full (CLUTTER_REPAINT_FLAGS_POST_PAINT,
                                         repaint_cb,
                                         stage,
                                         NULL);

  /* quit on destroy */
  g_signal_connect (stage, "destroy", G_CALLBACK (clutter_main_quit), NULL);



  clutter_main ();

  return EXIT_SUCCESS;
}
