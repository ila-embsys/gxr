openvr_sources = [
  'openvr-system.c',
  'openvr-overlay.c',
  'openvr-compositor.c',
  'openvr-context.c',
  'openvr-action.c',
  'openvr-action-set.c',
  'openvr-model.c',
  'openvr-math.c'
]

openvr_headers = [
  'openvr-system.h',
  'openvr-overlay.h',
  'openvr-compositor.h',
  'openvr-context.h',
  'openvr-action.h',
  'openvr-action-set.h',
  'openvr-model.h'
]

openvr_lib = static_library('gxr-openvr-' + api_version,
  openvr_sources,
  dependencies: gxr_deps + [
    openvr_dep,
    stdcpp_dep,
    dl_dep
  ],
  include_directories: gxr_inc,
  c_args : ['-DGXR_COMPILATION'],
  build_rpath : openvr_dep.get_pkgconfig_variable('libdir'),
  install_rpath : openvr_dep.get_pkgconfig_variable('libdir')
)

install_headers(openvr_headers, subdir: api_path)

gxr_backend_libs += [openvr_lib]
