# GNOME samples

examples_inc_dir = include_directories('../examples')

gtk_pixbuf = executable(
  'gtk_pixbuf', ['gtk_pixbuf.c', '../res/tests.c'],
  dependencies: gtk3_dep,
  install: false)

gtk_cairo = executable(
  'gtk_cairo', ['gtk_cairo.c'],
  dependencies: gtk3_dep,
  include_directories: examples_inc_dir,
  install: false)

gtk_cairo_animation = executable(
  'gtk_cairo_animation', ['gtk_cairo_animation.c'],
  dependencies: gtk3_dep,
  link_with: openvr_glib_lib,
  include_directories: [examples_inc_dir, openvr_glib_inc],
  install: false)

gtk_offscreen = executable(
  'gtk_offscreen', ['gtk_offscreen.c'],
  dependencies: gtk3_dep,
  install: false)

gdk_list_devices = executable(
  'gdk_list_devices', ['gdk_list_devices.c'],
  dependencies: gtk3_dep,
  install: false)

gdk_list_seats = executable(
  'gdk_list_seats', ['gdk_list_seats.c'],
  dependencies: gtk3_dep,
  install: false)

if clutter_gtk_dep.found()
  gtk_clutter = executable(
    'gtk_clutter', ['gtk_clutter.c'],
    dependencies: clutter_gtk_dep,
    include_directories: examples_inc_dir,
    install: false)
endif

if clutter_dep.found()
  clutter = executable(
    'clutter', ['clutter.c'],
    dependencies: clutter_dep,
    include_directories: examples_inc_dir,
    install: false)
endif

if cairo_dep.found()
  png_cairo = executable(
    'png_cairo', ['png_cairo.c'],
    dependencies: cairo_dep,
    include_directories: examples_inc_dir,
    install: false)
endif

if x11_dep.found()
  gtk_input_synth = executable(
    'gtk_input_synth', ['gtk_input_synth.c'],
    dependencies: [gtk3_dep, x11_dep, m_dep],
    install: false)
endif

if x11_dep.found()
  xsendevent_demo = executable(
    'xsendevent_demo', ['xsendevent_demo.c'],
    dependencies: [x11_dep],
    install: false)
endif

if x11_dep.found() and xi_dep.found()
  uinput_vrpointer = executable(
    'uinput-vrpointer', ['uinput-vrpointer.c'],
    dependencies: [x11_dep, xi_dep, m_dep],
    install: false)
endif
