gxr
===

A GLib wrapper for the OpenVR API. OpenXR support coming soon.

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
meson build -Dapi_doc=true
$ ninja -C build gxr-doc
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
meson test -C build/ --no-suite gxr:xr
```

Since meson `0.46` the project name can be omitted from the test suite:
```
meson test -C build/ --no-suite xr

```

## Code of Conduct

Please note that this project is released with a Contributor Code of Conduct.
By participating in this project you agree to abide by its terms.

We follow the standard freedesktop.org code of conduct,
available at <https://www.freedesktop.org/wiki/CodeOfConduct/>,
which is based on the [Contributor Covenant](https://www.contributor-covenant.org).

Instances of abusive, harassing, or otherwise unacceptable behavior may be
reported by contacting:

* First-line project contacts:
  * Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
  * Christoph Haag <christoph.haag@collabora.com>
* freedesktop.org contacts: see most recent list at <https://www.freedesktop.org/wiki/CodeOfConduct/>

