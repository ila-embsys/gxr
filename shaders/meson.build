glslang = find_program('glslangValidator')

if glslang.found()
  shaders = ['texture.vert', 'texture.frag']

  foreach s : shaders
    r = run_command('glslangValidator', '-V', '-o', s + '.spv', s)
    if r.returncode() != 0
      message('Could not compile shaders:')
      message(r.stderr().strip())
      message(r.stdout().strip())
    endif
  endforeach

  shader_resources = gnome.compile_resources(
    'shader_resources', 'shaders.gresource.xml',
    source_dir : '.')

endif