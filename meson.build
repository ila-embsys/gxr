project('openvr-glib', 'c', version: '0.9.0',
  meson_version: '>= 0.42.0',
)

gnome = import('gnome')

config_h = configuration_data()
configure_file(
  output: 'openvr-glib-config.h',
  configuration: config_h,
)
add_project_arguments([
  ['-I' + meson.build_root(),
   #'-Werror',
   '-Wall',
   '-Wextra'
],
], language: ['c'])

# Paths
openvr_glib_prefix = get_option('prefix')
openvr_glib_libdir = join_paths(openvr_glib_prefix, get_option('libdir'))
openvr_glib_includedir = join_paths(openvr_glib_prefix, get_option('includedir'))
openvr_glib_datadir = join_paths(openvr_glib_prefix, get_option('datadir'))
src_inc = include_directories('src')

### Dependencies
c = meson.get_compiler('c')

## Required
openvr_dep = c.find_library('openvr_api')

# For a statically linked libopenvr_api we also need to link to the
# Standard C++ library and the implementation of dlopen()
stdcpp_dep = c.find_library('stdc++')
dl_dep = c.find_library('dl')

gulkan_dep = dependency('gulkan-0.10', version: '>= 0.10.0')
gdk_dep = dependency('gdk-3.0', version: '>= 3.22')

## Optional
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
subdir('doc')
