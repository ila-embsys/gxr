project('gxr', 'c', version: '0.13.99',
  meson_version: '>= 0.45.1',
)

gnome = import('gnome')

config_h = configuration_data()

project_args = ['-I' + meson.build_root(),
   #'-Werror',
   '-Wall',
   '-Wextra',
]

compiler = meson.get_compiler('c')
compiler_id = compiler.get_id()

if compiler_id == 'clang'
  project_args += [
    '-Weverything',
    '-Wno-reserved-id-macro',
    '-Wno-documentation',
    '-Wno-documentation-unknown-command',
    '-Wno-padded',
    '-Wno-overlength-strings',
    '-Wno-disabled-macro-expansion',
    '-Wno-atomic-implicit-seq-cst',
    '-Wno-gnu-empty-initializer',
    '-Wno-covered-switch-default',
    '-Wno-switch-enum',
    '-Wno-used-but-marked-unused',
    '-Wno-double-promotion',
    # variadic %f
    '-Wno-gnu-folding-constant'
    # openvr max length macros
  ]
endif

add_project_arguments([project_args], language: ['c'])

# Paths
gxr_prefix = get_option('prefix')
gxr_libdir = join_paths(gxr_prefix, get_option('libdir'))
gxr_includedir = join_paths(gxr_prefix, get_option('includedir'))
gxr_datadir = join_paths(gxr_prefix, get_option('datadir'))
src_inc = include_directories('src')

### Dependencies
c = meson.get_compiler('c')

## Required


m_dep = c.find_library('m')

gulkan_dep = dependency('gulkan-0.14', version: '>= 0.13.0')
gdk_dep = dependency('gdk-3.0', version: '>= 3.22')

## Optional
openxr_dep = dependency('openxr', required: false)
if openxr_dep.found()
  config_h.set('GXR_HAS_OPENXR', 1)
endif

openvr_dep = dependency('openvr', required: false)
if openvr_dep.found()
  config_h.set('GXR_HAS_OPENVR', 1)
  config_h.set('GXR_HAS_OVERLAYS', 1)
  # For a statically linked libopenvr_api we also need to link to the
  # Standard C++ library and the implementation of dlopen()
  stdcpp_dep = c.find_library('stdc++')
  dl_dep = c.find_library('dl')
endif

gtk3_dep = dependency('gtk+-3.0', version: '>= 3.22', required : false)
x11_dep = dependency('x11', required : false)
xtst_dep = dependency('xtst', required : false)
clutter_dep = dependency('clutter-1.0', required : false)
cairo_dep = dependency('cairo', required : false)
libdrm_dep = dependency('libdrm', required : false)
libdrm_amdgpu_dep = dependency('libdrm_amdgpu', required : false)
glew_dep = dependency('glew', required : false)
glfw_dep = dependency('glfw3', required : false)
egl_dep = dependency('egl', required : false)

subdir('src')
subdir('res')
subdir('tests')
subdir('examples')

if get_option('api_doc')
    subdir('doc')
endif
