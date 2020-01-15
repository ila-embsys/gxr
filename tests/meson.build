test_overlay_pixbuf = executable(
  'test_overlay', ['test_overlay.c', test_resources],
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)
test('test_overlay',
     test_overlay_pixbuf,
     suite: ['xr'],
     is_parallel : false)

test_io = executable(
  'test_io', ['test_io.c', test_resources],
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)
test('test_io', test_io)

test_context = executable(
  'test_context', ['test_context.c', test_resources],
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)
test('test_context',
     test_context,
     suite: ['xr'],
     is_parallel : false)

test_actions = executable(
  'test_actions', ['test_actions.c', test_resources],
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)
test('test_actions',
     test_actions,
     suite: ['xr'],
     is_parallel : false)
