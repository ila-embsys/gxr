test_context = executable(
  'test_context', 'test_context.c',
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)
test('test_context', test_context, suite: ['xr'])

test_system = executable(
  'test_system', 'test_system.c',
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)
test('test_system', test_system, suite: ['xr'])

test_overlay_pixbuf = executable(
  'test_overlay_pixbuf', ['test_overlay_pixbuf.c', test_resources],
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)
test('test_overlay_pixbuf', test_overlay_pixbuf, suite: ['xr'])

test_io = executable(
  'test_io', ['test_io.c', test_resources],
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)
test('test_io', test_io)

test_vulkan_uploader = executable(
  'test_vulkan_uploader', ['test_vulkan_uploader.c', test_resources],
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)
test('test_vulkan_uploader', test_vulkan_uploader, suite: ['xr'])
