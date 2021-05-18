subdir('shaders')

cube_sources = [
  'cube.c',
  cube_shader_resources,
]

executable(
  'cube', cube_sources,
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)
