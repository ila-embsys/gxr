so_version = 0
api_version = '0.13'
api_path = 'gxr-' + api_version

gxr_sources = [
  'gxr-time.c',
  'gxr-io.c',
  'gxr-context.c',
  'openvr-system.c',
  'openvr-overlay.c',
  'openvr-compositor.c',
  'openvr-context.c',
  'openvr-action.c',
  'openvr-action-set.c',
  'openvr-model.c',
  'openvr-math.c',
  'openxr-context.c'
]

gxr_headers = [
  'gxr.h',
  'gxr-enums.h',
  'gxr-time.h',
  'gxr-io.h',
  'gxr-context.h',
  'openvr-system.h',
  'openvr-overlay.h',
  'openvr-compositor.h',
  'openvr-context.h',
  'openvr-action.h',
  'openvr-action-set.h',
  'openvr-model.h',
  'openxr-context.h'
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
  input: 'gxr-version.h.in',
  output: 'gxr-version.h',
  configuration: version_conf,
  install_dir: join_paths(get_option('includedir'), api_path)
)

gxr_deps = [
  gulkan_dep,
  openvr_dep,
  gdk_dep,
  stdcpp_dep,
  dl_dep,
  m_dep,
  openxr_dep
]

gxr_lib = shared_library(api_path,
  gxr_sources,
  version: meson.project_version(),
  soversion: so_version,
  dependencies: gxr_deps,
  install: true,
  c_args : ['-DGXR_COMPILATION'],
  build_rpath : openvr_dep.get_pkgconfig_variable('libdir'),
  install_rpath : openvr_dep.get_pkgconfig_variable('libdir')
)

gxr_inc = include_directories('.')

gxr_dep = declare_dependency(
  sources: [],
  link_with: gxr_lib,
  include_directories: [ gxr_inc ],
  dependencies: gxr_deps,
)

install_headers(gxr_headers, subdir: api_path)

pkg = import('pkgconfig')

pkg.generate(
  description: 'GLib wrapper for OpenVR',
    libraries: gxr_lib,
         name: 'gxr',
     filebase: api_path,
      version: meson.project_version(),
      subdirs: api_path,
     requires: ['gulkan-' + api_version, 'gdk-3.0', 'openvr'],
  install_dir: join_paths(get_option('libdir'), 'pkgconfig')
)

if get_option('introspection')
  gxr_gir = gnome.generate_gir(
    gxr_lib,
    sources: gxr_sources + gxr_headers,
    namespace: 'Gxr',
    nsversion: api_version,
    identifier_prefix: 'Gxr',
    symbol_prefix: 'gxr',
    export_packages: api_path,
    includes: [ 'cairo-1.0', 'GdkPixbuf-2.0', 'Gdk-3.0'],
    header: 'gxr/gxr.h',
    install: true,
  )
endif
