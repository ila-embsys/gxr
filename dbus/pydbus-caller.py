#!/usr/bin/env python3

from pydbus import SessionBus

bus = SessionBus()

proxy = bus.get(
    "org.gnome.VR",
    "/org/gnome/VR"
)

print (proxy.Introspect())

print("Was:", proxy.IsAvailable)

proxy.IsAvailable = not proxy.IsAvailable

print("Is:", proxy.IsAvailable)
