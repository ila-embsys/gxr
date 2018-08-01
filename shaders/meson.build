glslang = find_program('glslangValidator')

if glslang.found()
  shaders = ['texture.vert', 'texture.frag']

  foreach s : shaders
    r = run_command('glslangValidator', '-V', '-o', s + '.spv', s)
    if r.returncode() != 0
      message('Could not compile shaders:')
      message(r.stderr().strip())
    endif
  endforeach

  # generate gresources
  r = run_command('glib-compile-resources',
                  '--target=shaders.c',
                  '--sourcedir=.',
                  '--generate-source',
                  'shaders.gresource.xml')

  if r.returncode() != 0
    message('Could not compile resources:')
    message(r.stderr().strip())
  endif

endif