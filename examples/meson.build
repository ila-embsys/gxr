# gxr samples

executable(
  'actions', ['actions.c', test_resources],
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)

executable(
  'parse_manifest', ['parse_manifest.c', test_resources],
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)

subdir('demo')
subdir('cube')
