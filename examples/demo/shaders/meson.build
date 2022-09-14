shaders = ['pointer.vert', 'pointer.frag',
           'cube.frag', 'cube.vert' ]

glslc = find_program('glslc', required : false)
if glslc.found()
  # Prefer shaderc
  cmd = [glslc]
else
  # Use glslang as fallback
  glslang = find_program('glslangValidator')
  if glslang.found()
    cmd = [glslang, '-V']
  endif
endif

foreach s : shaders
  r = run_command(cmd + ['-o', s + '.spv', s], check: true)
  if r.returncode() != 0
    message('Could not compile shaders:')
    message(r.stderr().strip())
    message(r.stdout().strip())
  endif
endforeach

demo_shader_resources = gnome.compile_resources(
  'demo_shader_resources', 'shaders.gresource.xml',
  source_dir : '.')
