subdir('shaders')

cube_sources = [
  'gxr-demo.c',
  'scene-background.c',
  'scene-object.c',
  'scene-pointer.c',
  'scene-cube.c',
  'scene-controller.c',
  demo_shader_resources,
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
  'gxr-demo', cube_sources,
  c_args: c_args,
  dependencies: deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: true)
