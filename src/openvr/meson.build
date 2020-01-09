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

gxr_backend_libs += [openvr_lib]
