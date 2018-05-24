# GNOME samples

examples_inc_dir = include_directories('../examples')

cogl_triangle = executable(
  'cogl_triangle', ['cogl_triangle.c'],
  dependencies: cogl_1_dep,
  install: false)

cogl_cube = executable(
  'cogl_cube', ['cogl_cube.c', '../res/tests.c'],
  dependencies: [cogl_1_dep, gio_dep, gl_dep],
  install: false)

gtk_pixbuf = executable(
  'gtk_pixbuf', ['gtk_pixbuf.c', '../res/tests.c'],
  dependencies: gtk3_dep,
  install: false)

gtk_cairo = executable(
  'gtk_cairo', ['gtk_cairo.c'],
  dependencies: gtk3_dep,
  include_directories: examples_inc_dir,
  install: false)

gtk_offscreen = executable(
  'gtk_offscreen', ['gtk_offscreen.c'],
  dependencies: gtk3_dep,
  install: false)

gtk_offscreen = executable(
  'gtk_input_synth', ['gtk_input_synth.c'],
  dependencies: [gtk3_dep, x11_dep, m_dep],
  install: false)

gdk_list_devices = executable(
  'gdk_list_devices', ['gdk_list_devices.c'],
  dependencies: gtk3_dep,
  install: false)

gdk_list_seats = executable(
  'gdk_list_seats', ['gdk_list_seats.c'],
  dependencies: gtk3_dep,
  install: false)

gtk_clutter = executable(
  'gtk_clutter', ['gtk_clutter.c'],
  dependencies: clutter_gtk_dep,
  include_directories: examples_inc_dir,
  install: false)

clutter = executable(
  'clutter', ['clutter.c'],
  dependencies: clutter_dep,
  include_directories: examples_inc_dir,
  install: false)

png_cairo = executable(
  'png_cairo', ['png_cairo.c'],
  dependencies: cairo_dep,
  include_directories: examples_inc_dir,
  install: false)