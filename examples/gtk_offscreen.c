#include <gtk/gtk.h>

static gboolean
offscreen_damage (GtkWidget      *widget,
                  GdkEventExpose *event,
                  gpointer       *data)
{
  GdkPixbuf * offscreen_pixbuf =
    gtk_offscreen_window_get_pixbuf ((GtkOffscreenWindow *)widget);

  if (offscreen_pixbuf != NULL)
    {
      int width = gdk_pixbuf_get_width (offscreen_pixbuf);
      int height = gdk_pixbuf_get_height (offscreen_pixbuf);

      g_print ("Saving pixbuf with dimes %dx%d\n", width, height);

      GError *error = NULL;
      gdk_pixbuf_save (offscreen_pixbuf, "offscreen.jpg", "jpeg", &error, NULL);

      if (error != NULL) {
        fprintf (stderr, "Unable write pixbuf: %s\n", error->message);
        g_error_free (error);
      }

    } else {
      fprintf (stderr, "Could not acquire pixbuf.\n");
    }

  return TRUE;
}

static gboolean
da_button_press (GtkWidget *area, GdkEventButton *event, GtkWidget *button)
{
  g_print ("button pressed.\n");
  return TRUE;
}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  GtkWidget *window = gtk_offscreen_window_new ();

  GtkWidget * time_label = gtk_label_new ("00:33:44");
  GtkWidget * fps_label = gtk_label_new ("50 fps");

  GtkWidget * box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  GtkWidget *button = gtk_button_new_with_label ("Button");

  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), time_label, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), fps_label, FALSE, FALSE, 0);

  gtk_widget_set_size_request (box, 200, 200);
  gtk_container_add (GTK_CONTAINER (window), box);

  gtk_widget_show_all (window);

  g_signal_connect (window, "damage-event",
                    G_CALLBACK (offscreen_damage), NULL);

  g_signal_connect (button, "button_press_event", G_CALLBACK (da_button_press),
                    button);

  // gtk_widget_queue_draw (window);

  gtk_main();

  return 0;
}
