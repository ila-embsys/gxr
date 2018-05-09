exe = executable('test_system', 'test_system.c',
                 dependencies: openvr_glib_deps,
                 link_with: openvr_glib_lib,
                 include_directories: openvr_glib_inc,
                 install: false)

test_overlay_deps = [
  dependency('glfw3'),
  openvr_glib_deps
]

exe = executable('test_overlay', ['test_overlay.c', '../res/tests.c'],
                 dependencies: test_overlay_deps,
                 link_with: openvr_glib_lib,
                 include_directories: openvr_glib_inc,
                 install: false)

test_pixbuf_deps = [
  openvr_glib_deps
]

exe = executable('test_pixbuf', ['test_pixbuf.c', '../res/tests.c'],
                 dependencies:test_pixbuf_deps,
                 link_with: openvr_glib_lib,
                 include_directories: openvr_glib_inc,
                 install: false)

'''
exe = executable(unit, unit + '.c',
                 dependencies: graphene_dep,
                 include_directories: graphene_inc,
                 c_args: common_cflags + [
                   '-DG_LOG_DOMAIN="Graphene-Test"',
                   '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_30',
                   '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_32',
                   '-DGLIB_DISABLE_DEPRECATION_WARNINGS',
                 ],
                 install: true,
                 install_dir: installed_test_bindir)
'''

'''
test(
  'test_system',
  executable('test_system', 'test_system.c', dependencies: deps),
  env: [
    'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
    'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
  ],
)
'''