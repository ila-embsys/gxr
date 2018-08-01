project('openvr-glib', 'c', version: '0.4.0',
  meson_version: '>= 0.42.0',
)

gnome = import('gnome')

config_h = configuration_data()
configure_file(
  output: 'openvr-glib-config.h',
  configuration: config_h,
)
add_project_arguments([
  '-I' + meson.build_root(),
], language: ['c'])

### Dependencies
cpp = meson.get_compiler('c')

## Required
openvr_dep = cpp.find_library('openvr_api')
m_dep = cpp.find_library('m')
cogl_1_dep = dependency('cogl-1.0')
gio_dep = dependency('gio-2.0', version: '>= 2.50')
gl_dep = dependency('GL')
gtk3_dep = dependency('gtk+-3.0', version: '>= 3.22')
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version: '>= 2.36')

vulkan_dep = dependency('vulkan')
graphene_dep = dependency('graphene-1.0')


## Optional
cogl_mutter_dep = dependency('mutter-cogl-2', required : false)
x11_dep = dependency('x11', required : false)
xi_dep = dependency('xi', required : false)
clutter_gtk_dep = dependency('clutter-gtk-1.0', required : false)
clutter_dep = dependency('clutter-1.0', required : false)
cairo_dep = dependency('cairo', required : false)
libdrm_dep = dependency('libdrm', required : false)
libdrm_amdgpu_dep = dependency('libdrm_amdgpu', required : false)
glfw_dep = dependency('glfw3', required : false)
###

subdir('src')
subdir('shaders')
subdir('res')
subdir('tests')
subdir('examples')
subdir('examples-ext')
subdir('dbus')
