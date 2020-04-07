subdir('shaders')

cube_sources = [
  'gxr-cube.c',
  'xrd-scene-background.c',
  'xrd-scene-model.c',
  'xrd-scene-object.c',
  'xrd-scene-pointer.c',
  'xrd-scene-pointer-tip.c',
  'xrd-scene-renderer.c',
  'xrd-scene-cube.c',
  shader_resources,
  test_resources
]

deps = gxr_deps

c_args = []
renderdoc = get_option('cube_renderdoc')
if renderdoc
  c_args += '-DRENDERDOC'
  dl_dep = meson.get_compiler('c').find_library('dl', required : true)
  deps += dl_dep
endif

executable(
  'gxr-cube', cube_sources,
  c_args: c_args,
  dependencies: deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: true)
