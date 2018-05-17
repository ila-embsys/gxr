#!/usr/bin/env python3

from gi.repository import Gio, GLib

bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)
proxy = Gio.DBusProxy.new_sync(bus,
                               Gio.DBusProxyFlags.DO_NOT_AUTO_START,
                               None,
                               'org.gnome.VR',
                               '/org/gnome/VR',
                               'org.gnome.VR',
                               None)

is_available = proxy.get_cached_property("IsAvailable")

print("Was:", is_available)

prop_proxy = Gio.DBusProxy.new_sync(bus,
                                    Gio.DBusProxyFlags.DO_NOT_AUTO_START,
                                    None,
                                    'org.gnome.VR',
                                    '/org/gnome/VR',
                                    'org.freedesktop.DBus.Properties',
                                    None)

toggle = not is_available.get_boolean()
set_variant = GLib.Variant('(ssv)',("org.gnome.VR", "IsAvailable",
                           GLib.Variant.new_boolean(toggle)))

prop_proxy.call_sync("Set", set_variant, Gio.DBusCallFlags.NONE, -1, None)

get_variant = GLib.Variant('(ss)',("org.gnome.VR", "IsAvailable"))

res = prop_proxy.call_sync("Get", get_variant, Gio.DBusCallFlags.NONE, -1, None)
print ("Is:", res)
