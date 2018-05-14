#include <time.h>

#include <glib.h>
#include <glib/gprintf.h>
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
_press_cb (GtkWidget *button, GdkEventButton *event, gpointer data)
{
  g_print ("button pressed.\n");
  return TRUE;
}

struct Labels
{
  GtkWidget *time_label;
  GtkWidget *fps_label;
  struct timespec last_time;
};

#define SEC_IN_MSEC_D 1000.0
#define SEC_IN_NSEC_L 1000000000L
#define SEC_IN_NSEC_D 1000000000.0

/* assuming a > b */
void
_substract_timespecs (struct timespec* a,
                      struct timespec* b,
                      struct timespec* out)
{
  out->tv_sec = a->tv_sec - b->tv_sec;

  if (a->tv_nsec < b->tv_nsec)
  {
    out->tv_nsec = a->tv_nsec + SEC_IN_NSEC_L - b->tv_nsec;
    out->tv_sec--;
  }
  else
  {
    out->tv_nsec = a->tv_nsec - b->tv_nsec;
  }
}

double
_timespec_to_double_s (struct timespec* time)
{
  return ((double) time->tv_sec + (time->tv_nsec / SEC_IN_NSEC_D));
}

static gboolean
_draw_cb (GtkWidget *widget, cairo_t *cr, struct Labels* labels)
{
  struct timespec now;
  if (clock_gettime (CLOCK_REALTIME, &now) != 0)
  {
    fprintf (stderr, "Could not read system clock\n");
    return 0;
  }

  struct timespec diff;
  _substract_timespecs (&now, &labels->last_time, &diff);

  double diff_s = _timespec_to_double_s (&diff);
  double diff_ms = diff_s * SEC_IN_MSEC_D;
  double fps = SEC_IN_MSEC_D / diff_ms;

  gchar time_str [50];
  gchar fps_str [50];

  g_sprintf (time_str, "<span font=\"24\">%.2ld:%.9ld</span>",
             now.tv_sec, now.tv_nsec);

  g_sprintf (fps_str, "FPS %.2f (%.2fms)", fps, diff_ms);

  gtk_label_set_markup (GTK_LABEL (labels->time_label), time_str);
  gtk_label_set_text (GTK_LABEL (labels->fps_label), fps_str);

  labels->last_time.tv_sec = now.tv_sec;
  labels->last_time.tv_nsec = now.tv_nsec;

  return FALSE;
}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  // GtkWidget *window = gtk_offscreen_window_new ();
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  struct Labels labels;

  if (clock_gettime (CLOCK_REALTIME, &labels.last_time) != 0)
  {
    fprintf (stderr, "Could not read system clock\n");
    return 0;
  }

  labels.time_label = gtk_label_new ("");
  labels.fps_label = gtk_label_new ("");

  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  GtkWidget *button = gtk_button_new_with_label ("Button");

  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), labels.time_label, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), labels.fps_label, FALSE, FALSE, 0);

  gtk_widget_set_size_request (window , 300, 200);
  gtk_container_add (GTK_CONTAINER (window), box);

  gtk_widget_show_all (window);

  //g_signal_connect (window, "damage-event",
  //                  G_CALLBACK (offscreen_damage), NULL);

  g_signal_connect (button, "button_press_event", G_CALLBACK (_press_cb), NULL);

  g_signal_connect (window, "draw", G_CALLBACK (_draw_cb), &labels);

  // gtk_widget_queue_draw (window);

  gtk_main();

  return 0;
}
