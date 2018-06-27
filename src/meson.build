api_version = '0.1'

openvr_glib_sources = [
  'openvr-system.c',
  'openvr-overlay.c',
  'openvr-vulkan-uploader.c',
  'openvr-global.c',
  'openvr-compositor.c'
]

openvr_glib_headers = [
  'openvr-glib.h',
  'openvr-system.h',
  'openvr-overlay.h',
  'openvr-vulkan-uploader.h',
  'openvr-global.h',
  'openvr-compositor.h'
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
  gio_dep,
  gdk_pixbuf_dep,
  gtk3_dep,
  gl_dep,
  cogl_1_dep,
  openvr_dep,
  vulkan_dep
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

if get_option('introspection')
  openvr_glib_gir = gnome.generate_gir(
    openvr_glib_lib,
    sources: openvr_glib_sources + openvr_glib_headers,
    namespace: 'OpenVR',
    nsversion: '0.1',
    identifier_prefix: 'OpenVR',
    symbol_prefix: 'openvr',
    export_packages: 'openvr-glib-0.1',
    includes: [ 'cairo-1.0', 'GdkPixbuf-2.0', 'Gdk-3.0', 'Cogl-1.0'],
    header: 'openvr-glib/openvr-glib.h',
    install: true,
  )
endif