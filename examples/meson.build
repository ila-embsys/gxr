# OpenVR GLib samples

executable(
  'overlay_pixbuf', ['overlay_pixbuf.c', test_resources],
  dependencies: [gtk3_dep, openvr_dep, cairo_dep, graphene_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

executable(
  'overlay_multi', ['overlay_multi.c', test_resources],
  dependencies: [gtk3_dep, openvr_dep, cairo_dep, graphene_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

executable(
  'overlay_pointer', ['overlay_pointer.c', test_resources],
  dependencies: [gtk3_dep, openvr_dep, cairo_dep, graphene_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

executable(
  'overlay_models', ['overlay_models.c', test_resources],
  dependencies: [gtk3_dep, openvr_dep, cairo_dep, graphene_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

executable(
  'overlay_move', ['overlay_move.c', test_resources],
  dependencies: [gtk3_dep, openvr_dep, cairo_dep, graphene_dep, m_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

executable(
  'overlay_cairo', ['overlay_cairo.c'],
  dependencies: [gtk3_dep, cairo_dep, graphene_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

executable(
  'overlay_cairo_animation', ['overlay_cairo_animation.c'],
  dependencies: [gtk3_dep, cairo_dep, graphene_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

executable(
  'overlay_gtk', ['overlay_gtk.c'],
  dependencies: [gtk3_dep, vulkan_dep, graphene_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

if clutter_dep.found()
  executable(
    'overlay_clutter', ['overlay_clutter.c'],
    dependencies: [openvr_glib_deps, clutter_dep, vulkan_dep, graphene_dep],
    link_with: openvr_glib_lib,
    include_directories: openvr_glib_inc,
    install: false)
endif

if libdrm_amdgpu_dep.found()
  executable(
    'overlay_dmabuf', ['overlay_dmabuf.c'],
    dependencies: [gtk3_dep, vulkan_dep, libdrm_dep,
                   libdrm_amdgpu_dep, graphene_dep],
    link_with: openvr_glib_lib,
    include_directories: openvr_glib_inc,
    install: false)

  if glfw_dep.found()
    executable(
      'glfw_dmabuf', ['glfw_dmabuf.c', shader_resources],
      dependencies: [gtk3_dep, vulkan_dep, libdrm_dep,
                     libdrm_amdgpu_dep, glfw_dep, graphene_dep],
      link_with: openvr_glib_lib,
      include_directories: openvr_glib_inc,
      install: false)
  endif
endif

if glfw_dep.found()
  executable(
    'glfw_pixbuf', ['glfw_pixbuf.c', test_resources, shader_resources],
    dependencies: [gtk3_dep, vulkan_dep, glfw_dep, graphene_dep],
    link_with: openvr_glib_lib,
    include_directories: openvr_glib_inc,
    install: false)
endif