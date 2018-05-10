overlay_pixbuf = executable(
  'overlay_pixbuf', ['overlay_pixbuf.c', '../res/tests.c'],
  dependencies: [dependency('glfw3'), openvr_glib_deps],
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
  include_directories: openvr_glib_inc,
  install: false)

png_cairo = executable(
  'png_cairo', ['png_cairo.c'],
  dependencies: dependency('cairo'),
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)