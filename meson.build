project('openvr-glib', 'c', 'cpp', version: '0.1.0',
  meson_version: '>= 0.40.0',
)

config_h = configuration_data()
configure_file(
  output: 'openvr-glib-config.h',
  configuration: config_h,
)
add_project_arguments([
  '-I' + meson.build_root(),
], language: ['cpp', 'c'])

### Dependencies
cpp = meson.get_compiler('cpp')

## Required
openvr_dep = cpp.find_library('openvr_api')
m_dep = cpp.find_library('m')
cogl_1_dep = dependency('cogl-1.0')
gio_dep = dependency('gio-2.0', version: '>= 2.50')
gl_dep = dependency('GL')
gtk3_dep = dependency('gtk+-3.0', version: '>= 3.22')
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version: '>= 2.36')

## Optional
cogl_mutter_dep = dependency('mutter-cogl-2', required : false)
x11_dep = dependency('x11', required : false)
clutter_gtk_dep = dependency('clutter-gtk-1.0', required : false)
clutter_dep = dependency('clutter-1.0', required : false)
cairo_dep = dependency('cairo', required : false)
###

subdir('src')
subdir('res')
subdir('tests')
subdir('examples')
subdir('examples-ext')
subdir('dbus')
