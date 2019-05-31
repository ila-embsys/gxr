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
Run all tests.
```
$ ninja -C build test
```

Don't run tests that require a running XR runtime.
```
meson test -C build/ --no-suite xr
```
