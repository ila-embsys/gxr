test_system = executable(
  'test_system', 'test_system.c',
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)
test('test_system', test_system)