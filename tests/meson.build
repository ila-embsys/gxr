if openvr_dep.found()
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

  test_vulkan_uploader = executable(
    'test_vulkan_uploader', ['test_vulkan_uploader.c', test_resources],
    dependencies: gxr_deps,
    link_with: gxr_lib,
    include_directories: gxr_inc,
    install: false)
  test('test_vulkan_uploader', test_vulkan_uploader, suite: ['xr'])

  test_io = executable(
    'test_io', ['test_io.c', test_resources],
    dependencies: gxr_deps,
    link_with: gxr_lib,
    include_directories: gxr_inc,
    install: false)
  test('test_io', test_io)
endif

test_gxr_context = executable(
  'test_gxr_context', ['test_gxr_context.c', test_resources],
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)
test('test_gxr_context', test_gxr_context, suite: ['xr'])
