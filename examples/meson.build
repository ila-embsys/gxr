# OpenVR GLib samples

overlay_pixbuf_vulkan = executable(
  'overlay_pixbuf_vulkan', ['overlay_pixbuf_vulkan.c', '../res/tests.c'],
  dependencies: [gtk3_dep, openvr_dep, cogl_1_dep, cairo_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

overlay_pixbuf_vulkan_multi = executable(
  'overlay_pixbuf_vulkan_multi',
  ['overlay_pixbuf_vulkan_multi.c', '../res/tests.c'],
  dependencies: [gtk3_dep, openvr_dep, cogl_1_dep, cairo_dep, graphene_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

executable(
  'overlay_cairo_vulkan', ['overlay_cairo_vulkan.c'],
  dependencies: [cogl_1_dep, gtk3_dep, gl_dep, cairo_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

executable(
  'overlay_cairo_vulkan_animation', ['overlay_cairo_vulkan_animation.c'],
  dependencies: [cogl_1_dep, gtk3_dep, gl_dep, cairo_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

overlay_gtk_vulkan = executable(
  'overlay_gtk_vulkan', ['overlay_gtk_vulkan.c'],
  dependencies: [cogl_1_dep, gtk3_dep, vulkan_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

if clutter_dep.found()
  overlay_clutter_vulkan = executable(
    'overlay_clutter_vulkan', ['overlay_clutter_vulkan.c'],
    dependencies: [openvr_glib_deps, clutter_dep, vulkan_dep],
    link_with: openvr_glib_lib,
    include_directories: openvr_glib_inc,
    install: false)
endif

if libdrm_amdgpu_dep.found()
  overlay_dmabuf_vulkan = executable(
    'overlay_dmabuf_vulkan', ['overlay_dmabuf_vulkan.c'],
    dependencies: [cogl_1_dep, gtk3_dep,
                   vulkan_dep, libdrm_dep, libdrm_amdgpu_dep],
    link_with: openvr_glib_lib,
    include_directories: openvr_glib_inc,
    install: false)

  if glfw_dep.found()
    glfw_dmabuf_vulkan = executable(
      'glfw_dmabuf_vulkan', ['glfw_dmabuf_vulkan.c', '../shaders/shaders.c'],
      dependencies: [gtk3_dep, vulkan_dep, libdrm_dep,
                     libdrm_amdgpu_dep, glfw_dep],
      link_with: openvr_glib_lib,
      include_directories: openvr_glib_inc,
      install: false)
  endif
endif

if glfw_dep.found()
  glfw_pixbuf_vulkan = executable(
    'glfw_pixbuf_vulkan', ['glfw_pixbuf_vulkan.c',
                           '../res/tests.c',
                           '../shaders/shaders.c'],
    dependencies: [cogl_1_dep, gtk3_dep,
                   vulkan_dep, glfw_dep],
    link_with: openvr_glib_lib,
    include_directories: openvr_glib_inc,
    install: false)
endif