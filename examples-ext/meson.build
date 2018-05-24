# GNOME samples

examples_inc_dir = include_directories('../examples')

cogl_triangle = executable(
  'cogl_triangle', ['cogl_triangle.c'],
  dependencies: [dependency('cogl-1.0')],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

cogl_cube = executable(
  'cogl_cube', ['cogl_cube.c', '../res/tests.c'],
  dependencies: [dependency('cogl-1.0'),
                 dependency('gio-2.0'),
                 dependency('GL')],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

gtk_pixbuf = executable(
  'gtk_pixbuf', ['gtk_pixbuf.c', '../res/tests.c'],
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

gtk_cairo = executable(
  'gtk_cairo', ['gtk_cairo.c'],
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: [openvr_glib_inc, examples_inc_dir],
  install: false)

gtk_offscreen = executable(
  'gtk_offscreen', ['gtk_offscreen.c'],
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

gtk_offscreen = executable(
  'gtk_input_synth', ['gtk_input_synth.c'],
  dependencies: [openvr_glib_deps, dependency('x11')],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

gdk_list_devices = executable(
  'gdk_list_devices', ['gdk_list_devices.c'],
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

gdk_list_seats = executable(
  'gdk_list_seats', ['gdk_list_seats.c'],
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

gtk_clutter = executable(
  'gtk_clutter', ['gtk_clutter.c'],
  dependencies: [openvr_glib_deps, dependency('clutter-gtk-1.0')],
  link_with: openvr_glib_lib,
  include_directories: [openvr_glib_inc, examples_inc_dir],
  install: false)

clutter = executable(
  'clutter', ['clutter.c'],
  dependencies: [openvr_glib_deps, dependency('clutter-1.0')],
  link_with: openvr_glib_lib,
  include_directories: [openvr_glib_inc, examples_inc_dir],
  install: false)

png_cairo = executable(
  'png_cairo', ['png_cairo.c'],
  dependencies: dependency('cairo'),
  link_with: openvr_glib_lib,
  include_directories: [openvr_glib_inc, examples_inc_dir],
  install: false)