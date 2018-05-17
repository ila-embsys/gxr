#!/usr/bin/env python3

from gi.repository import Gio
from gi.repository import GLib


from IPython import embed

def on_action_invoked(proxy):
  print("ActionInvoked!", proxy)

def on_notification_closed(proxy):
  print("NotificationClosed!", proxy)

def on_monitors_changed(proxy):
  print("MonitorsChanged!", proxy)

def some_cb(proxy, task, user_data):
  print("some cb got some data", proxy, task, user_data)
  embed()
  #proxy.MonitorsChanged.connect(on_monitors_changed)
  #proxy.connect("MonitorsChanged", on_monitors_changed)

def on_signal(proxy, address, signal_name, data):
  print("address", address)
  print("signal_name", signal_name)
  print("data", data)

def on_properties_changed(foo):
  print("on_properties_changed", foo)

loop = GLib.MainLoop()

#bus = Gio.bus_get(Gio.BusType.SESSION, None)
bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)
proxy = Gio.DBusProxy.new_sync(bus, Gio.DBusProxyFlags.NONE, None,
                               'org.gnome.VR',
                               '/org/gnome/VR',
                               'org.gnome.VR', None)
# res = proxy.call_sync('GetCapabilities', None, Gio.DBusCallFlags.NO_AUTO_START, 500, None)

#position_data = res[2]
#mode_data = res[1]

proxy.connect("g-properties-changed", on_properties_changed)
proxy.connect("g-signal", on_signal)

"""
for i in range(len(mode_data)):
  print(i, mode_data[i][2]["display-name"])
  print (position_data[i])

  for mode in mode_data[i][1]:
    current_info = mode[-1]
    if current_info and "is-current" in current_info and current_info["is-current"]:
      print(mode)
"""      
      
# proxy.connect("ActionInvoked", on_action_invoked)
# proxy.connect("NotificationClosed", on_notification_closed)

#proxy.connect("MonitorsChanged", on_monitors_changed)

loop.run()

#embed()
