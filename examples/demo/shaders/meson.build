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

shader_targets = []
foreach s : shaders
  shader_targets += custom_target(
    'shader @0@'.format(s),
    command : cmd + ['@INPUT@', '-o', '@OUTPUT@'],
    input : s,
    output : s + '.spv',
  )
endforeach

demo_shader_resources = gnome.compile_resources(
  'demo_shader_resources', 'shaders.gresource.xml',
  source_dir : '.',
  dependencies: shader_targets
)
