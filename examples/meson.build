# gxr samples

executable(
  'overlay_pixbuf', ['overlay_pixbuf.c', test_resources],
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)

executable(
  'overlay_multi', ['overlay_multi.c', test_resources],
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)

executable(
  'overlay_multi_stresstest', ['overlay_multi_stresstest.c', test_resources],
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)

executable(
  'actions', ['actions.c', test_resources],
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)

executable(
  'overlay_cairo', ['overlay_cairo.c'],
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)

executable(
  'overlay_cairo_animation', ['overlay_cairo_animation.c'],
  dependencies: gxr_deps,
  link_with: gxr_lib,
  include_directories: gxr_inc,
  install: false)

if gtk3_dep.found()
  executable(
    'overlay_gtk', ['overlay_gtk.c'],
    dependencies: gxr_deps + [gtk3_dep],
    link_with: gxr_lib,
    include_directories: gxr_inc,
    install: false)

  executable(
    'keyboard',
    ['keyboard.c', test_resources],
    dependencies: gxr_deps + [gtk3_dep],
    link_with: gxr_lib,
    include_directories: gxr_inc,
    install: false)
endif

if x11_dep.found() and xtst_dep.found()
  executable(
    'x_keyboard_synthesis',
    ['x_keyboard_synthesis.c', test_resources],
    dependencies: gxr_deps + [x11_dep, xtst_dep],
    link_with: gxr_lib,
    include_directories: gxr_inc,
    install: false)
endif

if clutter_dep.found()
  executable(
    'overlay_clutter', ['overlay_clutter.c'],
    dependencies: gxr_deps + [clutter_dep],
    link_with: gxr_lib,
    include_directories: gxr_inc,
    install: false)
endif

if libdrm_amdgpu_dep.found()
  executable(
    'overlay_dmabuf', ['overlay_dmabuf.c'],
    dependencies: gxr_deps + [libdrm_dep, libdrm_amdgpu_dep],
    link_with: gxr_lib,
    include_directories: gxr_inc,
    install: false)
endif

if glew_dep.found() and glfw_dep.found() and egl_dep.found()
  executable(
    'overlay_mesa_dmabuf', ['overlay_mesa_dmabuf.c', test_resources],
    dependencies: gxr_deps + [glew_dep, glfw_dep, egl_dep],
    link_with: gxr_lib,
    include_directories: gxr_inc,
    install: false)

  executable(
    'overlay_external_memory', ['overlay_external_memory.c', test_resources],
    dependencies: gxr_deps + [glew_dep, glfw_dep, egl_dep],
    link_with: gxr_lib,
    include_directories: gxr_inc,
    install: false)
endif
