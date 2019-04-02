# openvr-glib

A glib wrapper and utilities for the OpenVR API

## Build

#### Configure the project
```
$ meson build
```

#### Compile the project
```
$ ninja -C build
```

#### Build the docs
```
$ ninja -C build openvr-glib-doc
```

## Run

#### Run the examples
```
$ ./build/examples/overlay_pixbuf
$ ./build/examples/overlay_cairo_vulkan_animation
$ ./build/examples/overlay_pixbuf_vulkan_multi
```

#### Run the tests
```
$ ninja -C build test
```

## DBUS

### Start DBUS server
```
$ ./build/dbus/gdbus-vr-server
```

### Toggle VR enabled status

SteamVR should be already running for this. Otherwise it will be launched.
Python 3 is required for this.

```
$ ./dbus/gdbus-vr-caller.py
```

## GNOME Shell

### Get current upload method
```
$ gsettings get org.gnome.shell.vr upload-method
```

### List upload methods
```
$ gsettings range org.gnome.shell.vr upload-method
```

### Set upload method
```
$ gsettings set org.gnome.shell.vr upload-method vk-upload-raw
```
