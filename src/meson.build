api_version = '0.1'

openvr_glib_sources = [
  'openvr-system.cpp',
]

openvr_glib_headers = [
  'openvr-glib.h',
  'openvr-system.h',
]

version_split = meson.project_version().split('.')
MAJOR_VERSION = version_split[0]
MINOR_VERSION = version_split[1]
MICRO_VERSION = version_split[2]

version_conf = configuration_data()
version_conf.set('VERSION', meson.project_version())
version_conf.set('MAJOR_VERSION', MAJOR_VERSION)
version_conf.set('MINOR_VERSION', MINOR_VERSION)
version_conf.set('MICRO_VERSION', MICRO_VERSION)

configure_file(
  input: 'openvr-glib-version.h.in',
  output: 'openvr-glib-version.h',
  configuration: version_conf,
  install: true,
  install_dir: join_paths(get_option('includedir'), 'openvr-glib')
)

openvr_glib_deps = [
  dependency('gio-2.0', version: '>= 2.50'),
]

openvr_glib_lib = shared_library('openvr-glib-' + api_version,
  openvr_glib_sources,
  dependencies: openvr_glib_deps,
  install: true,
)

openvr_glib_inc = include_directories('.')

openvr_glib_dep = declare_dependency(
  sources: [],
  link_with: openvr_glib_lib,
  include_directories: [ openvr_glib_inc ],
  dependencies: openvr_glib_deps,
)

install_headers(openvr_glib_headers, subdir: 'openvr-glib')

pkg = import('pkgconfig')

pkg.generate(
  description: 'GLib wrapper for OpenVR',
    libraries: openvr_glib_lib,
         name: 'openvr-glib',
     filebase: 'openvr-glib-' + api_version,
      version: meson.project_version(),
      subdirs: 'openvr-glib',
     requires: 'glib-2.0',
  install_dir: join_paths(get_option('libdir'), 'pkgconfig')
)
