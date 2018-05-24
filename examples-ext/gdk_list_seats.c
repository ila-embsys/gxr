/*
 * OpenVR GLib
 * Copyright 2018 Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gtk/gtk.h>

void
print_seat_info (GdkSeat *seat)
{
  g_print ("\n");
  GdkDevice *pointer = gdk_seat_get_pointer (seat);
  g_print ("pointer: %s\n", gdk_device_get_name (pointer));

  GdkDevice *keyboard = gdk_seat_get_keyboard (seat);
  g_print ("keyboard: %s\n", gdk_device_get_name (keyboard));

  GList *slaves = gdk_seat_get_slaves (seat, GDK_SEAT_CAPABILITY_ALL);
  GList *sl;
  for (sl = slaves; sl; sl = sl->next)
  {
    GdkDevice *slave = sl->data;
    g_print ("slave: %s\n", gdk_device_get_name (slave));
  }
  g_list_free (slaves);
}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);
  GtkWidget *gtk_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_show_all (gtk_window);

  GdkWindow *gdk_window = gtk_widget_get_window (gtk_window);
  GdkDisplay *display = gdk_window_get_display (gdk_window);

  GdkSeat *default_seat = gdk_display_get_default_seat (display);
  g_print ("\n======= DEFAULT SEAT =======\n");
  print_seat_info (default_seat);

  g_print ("\n======= ALL SEATS =======\n");
  GList *seats = gdk_display_list_seats (display);
  GList *s;
  for (s = seats; s; s = s->next)
  {
    GdkSeat *seat = s->data;
    print_seat_info (seat);
  }
  g_list_free (seats);

  return 0;
}
