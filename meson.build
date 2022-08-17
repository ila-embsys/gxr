project('gxr', 'c', version: '0.16.0',
        meson_version: '>= 0.52.0',
        default_options : [
          'c_std=c11',
          'warning_level=3',
        ],
)

gnome = import('gnome')

config_h = configuration_data()

project_args = ['-I' + meson.build_root(),
                '-Wno-overlength-strings'
]

compiler = meson.get_compiler('c')
compiler_id = compiler.get_id()

if compiler_id == 'clang'
  project_args += [
    '-Weverything',
    '-Wno-float-equal',
    '-Wno-reserved-id-macro',
    '-Wno-documentation',
    '-Wno-documentation-unknown-command',
    '-Wno-padded',
    '-Wno-disabled-macro-expansion',
    '-Wno-atomic-implicit-seq-cst',
    '-Wno-gnu-empty-initializer',
    '-Wno-covered-switch-default',
    '-Wno-switch-enum',
    '-Wno-used-but-marked-unused',
    '-Wno-double-promotion',
    '-Wno-format-nonliteral',
    # variadic %f
    '-Wno-gnu-folding-constant',
    '-Wno-cast-qual', # G_DEFINE_TYPE produces this
    '-Wno-declaration-after-statement',
    '-Wno-reserved-identifier',
    '-Wno-unused-macros', # GLib
  ]
endif

add_project_arguments([project_args], language: ['c'])

# Paths
gxr_prefix = get_option('prefix')
gxr_libdir = join_paths(gxr_prefix, get_option('libdir'))
gxr_includedir = join_paths(gxr_prefix, get_option('includedir'))
gxr_datadir = join_paths(gxr_prefix, get_option('datadir'))
src_inc = include_directories('src')

### Dependencies
c = meson.get_compiler('c')

## Required
m_dep = c.find_library('m')

gulkan_dep_major_minor_ver = '0.16'
gulkan_dep_patch_ver = '.0'

gulkan_proj = subproject('gulkan',
  default_options: ['api_doc=false', 'tests=false', 'examples=false'],
  required: false,
  version: '>= ' + gulkan_dep_major_minor_ver + gulkan_dep_patch_ver,
)

if gulkan_proj.found()
  gulkan_dep = gulkan_proj.get_variable('gulkan_dep')
else
  gulkan_dep = dependency('gulkan-' + gulkan_dep_major_minor_ver, version: '>= ' + gulkan_dep_major_minor_ver + gulkan_dep_patch_ver,
    include_type: 'system')
endif

gdk_dep = dependency('gdk-3.0', version: '>= 3.22')
gmodule_dep = dependency('gmodule-2.0')
json_glib_dep = dependency('json-glib-1.0')

openxr_dep = dependency('openxr')
config_h.set('GXR_DEFAULT_API', 'GXR_API_OPENXR')

subdir('src')
subdir('res')

if get_option('tests')
	subdir('tests')
endif

if get_option('examples')
    subdir('examples')
endif

if get_option('api_doc')
    subdir('doc')
endif
