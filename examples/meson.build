# OpenVR GLib samples

overlay_pixbuf = executable(
  'overlay_pixbuf', ['overlay_pixbuf.c', '../res/tests.c'],
  dependencies: [dependency('cogl-1.0'),
                 dependency('gtk+-3.0'),
                 dependency('GL')],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

overlay_pixbuf_mutter_cogl = executable(
  'overlay_pixbuf_mutter', ['overlay_pixbuf.c', '../res/tests.c'],
  dependencies: [dependency('mutter-cogl-2'),
                 dependency('gtk+-3.0'),
                 dependency('GL')],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

overlay_cogl_texture = executable(
  'overlay_cogl_texture', ['overlay_cogl_texture.c', '../res/tests.c'],
  dependencies: [dependency('cogl-1.0'),
                 dependency('gtk+-3.0'),
                 dependency('GL')],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

overlay_cairo = executable(
  'overlay_cairo', ['overlay_cairo.c'],
  dependencies: [dependency('cogl-1.0'),
                 dependency('gtk+-3.0'),
                 dependency('GL'),
                 dependency('cairo')],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

overlay_gtk = executable(
  'overlay_gtk', ['overlay_gtk.c', '../res/tests.c'],
  dependencies: [dependency('cogl-1.0'),
                 dependency('gtk+-3.0'),
                 dependency('GL')],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

overlay_clutter = executable(
  'overlay_clutter', ['overlay_clutter.c'],
  dependencies: [openvr_glib_deps, dependency('clutter-1.0')],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)