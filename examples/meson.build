# OpenVR GLib samples

overlay_pixbuf = executable(
  'overlay_pixbuf', ['overlay_pixbuf.c', '../res/tests.c'],
  dependencies: [cogl_1_dep, gtk3_dep, gl_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

overlay_pixbuf_raw = executable(
  'overlay_pixbuf_raw', ['overlay_pixbuf_raw.c', '../res/tests.c'],
  dependencies: [cogl_1_dep, gtk3_dep, gl_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

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

overlay_cogl_texture = executable(
  'overlay_cogl_texture', ['overlay_cogl_texture.c', '../res/tests.c'],
  dependencies: [cogl_1_dep, gtk3_dep, gl_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

overlay_cairo = executable(
  'overlay_cairo', ['overlay_cairo.c'],
  dependencies: [cogl_1_dep, gtk3_dep, gl_dep, cairo_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

overlay_cairo_raw = executable(
  'overlay_cairo_raw', ['overlay_cairo_raw.c'],
  dependencies: [cogl_1_dep, gtk3_dep, gl_dep, cairo_dep],
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

overlay_gtk = executable(
  'overlay_gtk', ['overlay_gtk.c'],
  dependencies: [cogl_1_dep, gtk3_dep, gl_dep],
  link_with: openvr_glib_lib,
  include_directories: openvr_glib_inc,
  install: false)

if clutter_dep.found()
  overlay_clutter = executable(
    'overlay_clutter', ['overlay_clutter.c'],
    dependencies: [openvr_glib_deps, clutter_dep],
    link_with: openvr_glib_lib,
    include_directories: openvr_glib_inc,
    install: false)
endif

if cogl_mutter_dep.found()
  overlay_pixbuf_mutter_cogl = executable(
    'overlay_pixbuf_mutter', ['overlay_pixbuf.c', '../res/tests.c'],
    dependencies: [cogl_mutter_dep, gtk3_dep, gl_dep],
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
endif

if glfw_dep.found()
  glfw_pixbuf_vulkan = executable(
    'glfw_pixbuf_vulkan', ['glfw_pixbuf_vulkan.c', '../res/tests.c'],
    dependencies: [cogl_1_dep, gtk3_dep,
                   vulkan_dep, glfw_dep],
    link_with: openvr_glib_lib,
    include_directories: openvr_glib_inc,
    install: false)
endif