# OpenVR GLib samples

executable(
  'overlay_pixbuf', ['overlay_pixbuf.c', test_resources],
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

executable(
  'overlay_multi', ['overlay_multi.c', test_resources],
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

executable(
  'actions', ['actions.c', test_resources],
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

executable(
  'overlay_cairo', ['overlay_cairo.c'],
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

executable(
  'overlay_cairo_animation', ['overlay_cairo_animation.c'],
  dependencies: openvr_glib_deps,
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

if gtk3_dep.found()
  executable(
    'overlay_gtk', ['overlay_gtk.c'],
    dependencies: openvr_glib_deps + [gtk3_dep],
    link_with: openvr_glib_lib,
    include_directories: openvr_glib_inc,
    install: false)

  executable(
    'keyboard',
    ['keyboard.c', test_resources],
    dependencies: openvr_glib_deps + [gtk3_dep],
    link_with: openvr_glib_lib,
    include_directories: openvr_glib_inc,
    install: false)
endif

if x11_dep.found() and xtst_dep.found()
  executable(
    'x_keyboard_synthesis',
    ['x_keyboard_synthesis.c', test_resources],
    dependencies: openvr_glib_deps + [x11_dep, xtst_dep],
    link_with: openvr_glib_lib,
    include_directories: openvr_glib_inc,
    install: false)
endif

if clutter_dep.found()
  executable(
    'overlay_clutter', ['overlay_clutter.c'],
    dependencies: openvr_glib_deps + [clutter_dep],
    link_with: openvr_glib_lib,
    include_directories: openvr_glib_inc,
    install: false)
endif

if libdrm_amdgpu_dep.found()
  executable(
    'overlay_dmabuf', ['overlay_dmabuf.c'],
    dependencies: openvr_glib_deps + [libdrm_dep, libdrm_amdgpu_dep],
    link_with: openvr_glib_lib,
    include_directories: openvr_glib_inc,
    install: false)
endif

if glew_dep.found() and glfw_dep.found() and egl_dep.found()
  executable(
    'overlay_mesa_dmabuf', ['overlay_mesa_dmabuf.c', test_resources],
    dependencies: openvr_glib_deps + [glew_dep, glfw_dep, egl_dep],
    link_with: openvr_glib_lib,
    include_directories: openvr_glib_inc,
    install: false)

  executable(
    'overlay_external_memory', ['overlay_external_memory.c', test_resources],
    dependencies: openvr_glib_deps + [glew_dep, glfw_dep, egl_dep],
    link_with: openvr_glib_lib,
    include_directories: openvr_glib_inc,
    install: false)
endif
