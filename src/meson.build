so_version = 0
api_version = '0.14'
api_path = 'gxr-' + api_version

gxr_sources = [
  'gxr-time.c',
  'gxr-context.c',
  'gxr-action.c',
  'gxr-action-set.c',
  'gxr-io.c',
  'gxr-overlay.c',
  'gxr-overlay-model.c'
]

gxr_headers = [
  'gxr.h',
  'gxr-enums.h',
  'gxr-types.h',
  'gxr-time.h',
  'gxr-context.h',
  'gxr-action.h',
  'gxr-action-set.h',
  'gxr-io.h',
  'gxr-overlay.h',
  'gxr-overlay-model.h'
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
  output: 'gxr-config.h',
  configuration: config_h,
  install_dir: join_paths(get_option('includedir'), api_path)
)

configure_file(
  input: 'gxr-version.h.in',
  output: 'gxr-version.h',
  configuration: version_conf,
  install_dir: join_paths(get_option('includedir'), api_path)
)

gxr_deps = [
  gulkan_dep,
  gdk_dep,
  m_dep,
]

gxr_inc = include_directories('.')

gxr_backend_libs = []

if openvr_dep.found()
  subdir('openvr')
endif
if openxr_dep.found()
  subdir('openxr')
endif

gxr_lib = shared_library(api_path,
  gxr_sources,
  version: meson.project_version(),
  soversion: so_version,
  dependencies: gxr_deps,
  include_directories: gxr_inc,
  install: true,
  link_whole: gxr_backend_libs,
  c_args : ['-DGXR_COMPILATION']
)

gxr_dep = declare_dependency(
  sources: [],
  link_with: gxr_lib,
  include_directories: gxr_inc,
  dependencies: gxr_deps,
)

install_headers(gxr_headers, subdir: api_path)

pkg = import('pkgconfig')

pkg_requires = ['gulkan-' + api_version, 'gdk-3.0']
if openvr_dep.found()
  pkg_requires += ['openvr']
endif

pkg.generate(
  description: 'GLib abtration for XR APIs.',
    libraries: gxr_lib,
         name: 'gxr',
     filebase: api_path,
      version: meson.project_version(),
      subdirs: api_path,
     requires: pkg_requires,
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
