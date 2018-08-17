#include <gio/gio.h>
#include <stdlib.h>


static GDBusNodeInfo *introspection_data = NULL;
static gboolean is_available = FALSE;

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.gnome.VR'>"
  "    <property type='b' name='IsAvailable' access='readwrite'/>"
  "  </interface>"
  "</node>";


static GVariant *
handle_get_property (GDBusConnection *connection,
                     const gchar     *sender,
                     const gchar     *object_path,
                     const gchar     *interface_name,
                     const gchar     *property_name,
                     GError         **error,
                     gpointer         user_data)
{
  (void) connection;
  (void) sender;
  (void) object_path;
  (void) interface_name;
  (void) error;
  (void) user_data;

  GVariant *ret;
  ret = NULL;
  if (g_strcmp0 (property_name, "IsAvailable") == 0)
    {
      ret = g_variant_new_boolean (is_available);
    }

  return ret;
}

static gboolean
handle_set_property (GDBusConnection *connection,
                     const gchar     *sender,
                     const gchar     *object_path,
                     const gchar     *interface_name,
                     const gchar     *property_name,
                     GVariant        *value,
                     GError         **error,
                     gpointer         user_data)
{
  (void) sender;
  (void) user_data;
  if (g_strcmp0 (property_name, "IsAvailable") == 0)
    {
      GVariantBuilder *builder;
      GError *local_error;

      if (is_available != g_variant_get_boolean (value))
        {
          is_available = g_variant_get_boolean (value);

          local_error = NULL;
          builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
          g_variant_builder_add (builder,
                                 "{sv}",
                                  "IsAvailable",
                                 g_variant_new_boolean (is_available));
          g_dbus_connection_emit_signal (connection,
                                         NULL,
                                         object_path,
                                         "org.freedesktop.DBus.Properties",
                                         "PropertiesChanged",
                                         g_variant_new ("(sa{sv}as)",
                                                        interface_name,
                                                        builder,
                                                        NULL),
                                         &local_error);
          g_assert_no_error (local_error);
        }
    }

  return *error == NULL;
}


static const GDBusInterfaceVTable interface_vtable =
{
  .method_call = NULL,
  .get_property = handle_get_property,
  .set_property = handle_set_property,
  .padding = {0}
};

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
  (void) name;
  (void) user_data;

  guint registration_id;

  registration_id =
    g_dbus_connection_register_object (connection,
                                       "/org/gnome/VR",
                                       introspection_data->interfaces[0],
                                       &interface_vtable,
                                       NULL,  /* user_data */
                                       NULL,  /* user_data_free_func */
                                       NULL); /* GError** */
  g_assert (registration_id > 0);
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
  (void) connection;
  (void) name;
  (void) user_data;
  g_print ("Name %s acquired.\n", name);
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  (void) connection;
  (void) user_data;
  g_print ("Name %s lost. Bye.\n", name);
  exit (1);
}

int
main ()
{
  guint owner_id;
  GMainLoop *loop;

  introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  g_assert (introspection_data != NULL);

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             "org.gnome.VR",
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             NULL,
                             NULL);

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);

  g_bus_unown_name (owner_id);

  g_dbus_node_info_unref (introspection_data);

  return 0;
}
