test_system = executable(
  'test_system', 'test_system.c',
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)
test('test_system', test_system)

test_overlay_pixbuf = executable(
  'test_overlay_pixbuf', ['test_overlay_pixbuf.c', '../res/tests.c'],
  dependencies: [cogl_1_dep, openvr_glib_deps],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)
test('test_overlay_pixbuf', test_overlay_pixbuf)

test_vulkan_instance = executable(
  'test_vulkan_instance', ['test_vulkan_instance.c'],
  dependencies: [vulkan_dep, openvr_glib_deps],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)
test('test_vulkan_instance', test_vulkan_instance)

test_vulkan_device = executable(
  'test_vulkan_device', ['test_vulkan_device.c'],
  dependencies: [vulkan_dep, openvr_glib_deps],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)
test('test_vulkan_device', test_vulkan_device)

test_vulkan_client = executable(
  'test_vulkan_client', ['test_vulkan_client.c'],
  dependencies: [vulkan_dep, openvr_glib_deps],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)
test('test_vulkan_client', test_vulkan_client)

test_vulkan_texture = executable(
  'test_vulkan_texture', ['test_vulkan_texture.c', '../res/tests.c'],
  dependencies: [vulkan_dep, openvr_glib_deps],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)
test('test_vulkan_texture', test_vulkan_texture)