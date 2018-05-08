# generate gresources
r = run_command('glib-compile-resources',
                  '--target=tests.c',
                  '--sourcedir=.',
                  '--generate-source',
                  'tests.gresource.xml')

if r.returncode() != 0
  message('Could not compile resources:')
  message(r.stderr().strip())
endif