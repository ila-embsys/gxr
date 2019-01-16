so_version = 0
api_version = '0.8'
api_path = 'openvr-glib-' + api_version

openvr_glib_sources = [
  'openvr-system.c',
  'openvr-overlay.c',
  'openvr-compositor.c',
  'openvr-time.c',
  'openvr-math.c',
  'openvr-context.c',
  'openvr-action.c',
  'openvr-action-set.c',
  'openvr-io.c',
  'openvr-overlay-uploader.c',
  'xrd-overlay-model.c',
  'xrd-overlay-button.c',
  'xrd-overlay-pointer-tip.c',
  'xrd-overlay-pointer.c',
  'xrd-overlay-manager.c',
  'xrd-scene-model.c',
  'xrd-scene-device-manager.c',
  'xrd-scene-device.c',
  'xrd-scene-client.c',
  'xrd-scene-window.c',
  'xrd-scene-pointer.c',
  'xrd-scene-object.c'
]

openvr_glib_headers = [
  'openvr-glib.h',
  'openvr-system.h',
  'openvr-overlay.h',
  'openvr-compositor.h',
  'openvr-time.h',
  'openvr-math.h',
  'openvr-context.h',
  'openvr-action.h',
  'openvr-action-set.h',
  'openvr-io.h',
  'openvr-overlay-uploader.h',
  'xrd-overlay-model.h',
  'xrd-overlay-button.h',
  'xrd-overlay-pointer-tip.h',
  'xrd-overlay-pointer.h',
  'xrd-overlay-manager.h',
  'xrd-scene-model.h',
  'xrd-scene-device-manager.h',
  'xrd-scene-device.h',
  'xrd-scene-client.h',
  'xrd-scene-window.h',
  'xrd-scene-pointer.h',
  'xrd-scene-object.h'
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
  install_dir: join_paths(get_option('includedir'), api_path)
)

openvr_deps = [
  openvr_dep,
  stdcpp_dep,
  dl_dep,
]

openvr_glib_deps = openvr_deps + [
  gio_dep,
  gdk_pixbuf_dep,
  gtk3_dep,
  vulkan_dep,
  graphene_dep,
  m_dep,
  gulkan_dep
]

openvr_glib_lib = shared_library(api_path,
  openvr_glib_sources,
  version: meson.project_version(),
  soversion: so_version,
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

install_headers(openvr_glib_headers, subdir: api_path)

pkg = import('pkgconfig')

pkg.generate(
  description: 'GLib wrapper for OpenVR',
    libraries: openvr_glib_lib,
         name: 'openvr-glib',
     filebase: api_path,
      version: meson.project_version(),
      subdirs: api_path,
     requires: 'glib-2.0',
  install_dir: join_paths(get_option('libdir'), 'pkgconfig')
)

if get_option('introspection')
  openvr_glib_gir = gnome.generate_gir(
    openvr_glib_lib,
    sources: openvr_glib_sources + openvr_glib_headers,
    namespace: 'OpenVR',
    nsversion: api_version,
    identifier_prefix: 'OpenVR',
    symbol_prefix: 'openvr',
    export_packages: api_path,
    includes: [ 'cairo-1.0', 'GdkPixbuf-2.0', 'Gdk-3.0'],
    header: 'openvr-glib/openvr-glib.h',
    install: true,
  )
endif