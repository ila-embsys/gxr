subdir('xml')

private_headers = [
  'config.h',
  'openvr-glib.h',
  'openvr-glib-version.h'
]

glib_prefix = dependency('glib-2.0').get_pkgconfig_variable('prefix')
glib_docpath = join_paths(glib_prefix, 'share', 'gtk-doc', 'html')
docpath = join_paths(openvr_glib_datadir, 'gtk-doc', 'html')

gnome.gtkdoc('openvr-glib',
  main_xml: 'openvr-glib-docs.xml',
  src_dir: [
    src_inc,
  ],
  dependencies: openvr_glib_dep,
  gobject_typesfile: 'openvr-glib.types',
  scan_args: [
    '--rebuild-types',
    '--ignore-headers=' + ' '.join(private_headers),
  ],
  mkdb_args: [
    '--default-include=openvr-glib.h',
  ],
  fixxref_args: [
    '--html-dir=@0@'.format(docpath),
    '--extra-dir=@0@'.format(join_paths(glib_docpath, 'glib')),
    '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gobject')),
    '--extra-dir=@0@'.format(join_paths(glib_docpath, 'graphene')),
    '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gulkan')),
  ],
  # html_assets: html_images,
  install: true,
)
