openxr_api_path = 'gxr-openxr-' + api_version

openxr_sources = [
  'openxr-context.c',
  'openxr-action.c',
  'openxr-action-set.c',
  'openxr-overlay.c'
]

openxr_headers = [
  'openxr-interface.h',
  'openxr-context.h',
  'openxr-action.h',
  'openxr-action-set.h',
]

openxr_lib = static_library(openxr_api_path,
  openxr_sources,
  dependencies: gxr_deps + [openxr_dep],
  include_directories: gxr_inc,
  c_args : ['-DGXR_COMPILATION']
)

install_headers(openxr_headers, subdir: api_path)

gxr_backend_libs += [openxr_lib]
