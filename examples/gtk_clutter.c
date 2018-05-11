#include <gtk/gtk.h>
#include <clutter/clutter.h>
#include <math.h>

#include <clutter-gtk/clutter-gtk.h>

#include "clutter_content.h"

#define WINWIDTH   400
#define WINHEIGHT  400

int
main (int argc, char *argv[])
{
  ClutterActor    *stage;
  GtkWidget       *window, *clutter;

  ClutterInitError ret =
    gtk_clutter_init_with_args (&argc, &argv, NULL, NULL, NULL, NULL);

  if (ret != CLUTTER_INIT_SUCCESS)
    g_error ("Unable to initialize GtkClutter");

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  clutter = gtk_clutter_embed_new ();
  gtk_widget_set_size_request (clutter, WINWIDTH, WINHEIGHT);

  gtk_container_add (GTK_CONTAINER (window), clutter);

  stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (clutter));

  create_rotated_quad_stage (stage);

  gtk_widget_show_all (window);

  gtk_main();

  return 0;
}
