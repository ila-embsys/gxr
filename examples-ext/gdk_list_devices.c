/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gtk/gtk.h>

void
print_devices (GList *devices)
{
  GList *d;
  for (d = devices; d; d = d->next)
  {
    GdkDevice *device = d->data;
    g_print ("%s\n", gdk_device_get_name (device));
  }
}

void
print_device_list (GdkDeviceManager *device_manager,
                   const gchar      *name,
                   GdkDeviceType     type)
{
  GList *devices;
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  devices = gdk_device_manager_list_devices (device_manager, type);
  G_GNUC_END_IGNORE_DEPRECATIONS;
  g_print ("\n=== %s ===\n\n", name);
  print_devices (devices);
  g_list_free (devices);
}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);
  GtkWidget *gtk_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_show_all (gtk_window);

  GdkWindow *gdk_window = gtk_widget_get_window (gtk_window);
  GdkDisplay *display = gdk_window_get_display (gdk_window);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  GdkDeviceManager *device_manager = gdk_display_get_device_manager (display);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  print_device_list (device_manager,
                     "GDK_DEVICE_TYPE_MASTER",
                     GDK_DEVICE_TYPE_MASTER);

  print_device_list (device_manager,
                     "GDK_DEVICE_TYPE_SLAVE",
                     GDK_DEVICE_TYPE_SLAVE);

  print_device_list (device_manager,
                     "GDK_DEVICE_TYPE_FLOATING",
                     GDK_DEVICE_TYPE_FLOATING);

  return 0;
}
