test_system = executable(
  'test_system', 'test_system.c',
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)
test('test_system', test_system)

test_overlay_deps = [
  dependency('glfw3'),
  openvr_glib_deps
]

test_overlay_pixbuf = executable('test_overlay_pixbuf',
  ['test_overlay_pixbuf.c', '../res/tests.c'],
  dependencies: test_overlay_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)
test('test_overlay_pixbuf', test_overlay_pixbuf)

test_gtk_pixbuf = executable(
  'test_gtk_pixbuf',
  ['test_gtk_pixbuf.c', '../res/tests.c'],
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)
test('test_gtk_pixbuf', test_gtk_pixbuf)