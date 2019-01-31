test_context = executable(
  'test_context', 'test_context.c',
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)
test('test_context', test_context)

test_system = executable(
  'test_system', 'test_system.c',
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)
test('test_system', test_system)

test_overlay_pixbuf = executable(
  'test_overlay_pixbuf', ['test_overlay_pixbuf.c', test_resources],
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)
test('test_overlay_pixbuf', test_overlay_pixbuf)

test_io = executable(
  'test_io', ['test_io.c', test_resources],
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)
test('test_io', test_io)

test_vulkan_uploader = executable(
  'test_vulkan_uploader', ['test_vulkan_uploader.c', test_resources],
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)
test('test_vulkan_uploader', test_vulkan_uploader)
