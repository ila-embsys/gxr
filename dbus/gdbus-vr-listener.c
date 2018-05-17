#include <gio/gio.h>

static gchar *opt_name         = "org.gnome.VR";
static gchar *opt_object_path  = "/org/gnome/VR";
static gchar *opt_interface    = "org.gnome.VR";

static void
print_property (GDBusProxy *proxy)
{
  GVariant *value = g_dbus_proxy_get_cached_property (proxy, "IsAvailable");
  gchar *value_str = g_variant_print (value, TRUE);
  g_print ("IsAvailable: %s\n", value_str);

  g_variant_unref (value);
  g_free (value_str);
}

static void
on_properties_changed (GDBusProxy          *proxy,
                       GVariant            *changed_properties,
                       const gchar* const  *invalidated_properties,
                       gpointer             user_data)
{
  if (g_variant_n_children (changed_properties) > 0)
    {
      GVariant *value = g_variant_lookup_value (changed_properties,
                                                "IsAvailable",
                                                G_VARIANT_TYPE_BOOLEAN);
      if (value != NULL)
        {
          gchar *value_str = g_variant_print (value, TRUE);
          g_print ("IsAvailable %s\n", value_str);
          g_variant_unref (value);
          g_free (value_str);
        }
    }
}

static void
print_proxy (GDBusProxy *proxy)
{
  gchar *name_owner = g_dbus_proxy_get_name_owner (proxy);
  if (name_owner != NULL)
    {
      g_print ("+++ %s is owned by %s +++\n", opt_name, name_owner);
      print_property (proxy);
    }
  else
    {
      g_print ("+++ No owner for %s +++\n", opt_name);
    }
  g_free (name_owner);
}

static void
on_name_owner_notify (GObject    *object,
                      GParamSpec *pspec,
                      gpointer    user_data)
{
  GDBusProxy *proxy = G_DBUS_PROXY (object);
  print_proxy (proxy);
}

int
main (int argc, char *argv[])
{
  GError *error = NULL;
  GDBusProxyFlags flags;
  GDBusProxy *proxy;
  GMainLoop *loop = NULL;

  flags = G_DBUS_PROXY_FLAGS_NONE | G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START;

  loop = g_main_loop_new (NULL, FALSE);

  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         flags,
                                         NULL, /* GDBusInterfaceInfo */
                                         opt_name,
                                         opt_object_path,
                                         opt_interface,
                                         NULL, /* GCancellable */
                                         &error);
  if (proxy == NULL)
    {
      g_printerr ("Error creating proxy: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      g_signal_connect (proxy,
                        "g-properties-changed",
                        G_CALLBACK (on_properties_changed),
                        NULL);
      g_signal_connect (proxy,
                        "notify::g-name-owner",
                        G_CALLBACK (on_name_owner_notify),
                        NULL);
      print_proxy (proxy);

      g_main_loop_run (loop);
    }

  if (proxy != NULL)
    g_object_unref (proxy);
  if (loop != NULL)
    g_main_loop_unref (loop);
  g_free (opt_name);
  g_free (opt_object_path);
  g_free (opt_interface);

  return 0;
}
