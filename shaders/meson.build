# Define the compiler up-front so submodules can access it.
glslc = find_program('glslc')

shaders = [
  'cas.frag',
  'fullscreen.vert',
  'greyscale.frag',
  'simple_grid.frag',
]

shader_defines = []
shader_targets = []
foreach shader : shaders
  shader_targets += custom_target(
    '@0@'.format(shader),
    command : [glslc, shader_defines, '-o', '@OUTPUT@', '@INPUT@'],
    input : shader,
    output : '@PLAINNAME@.spv',
    install : true,
    install_dir : datadir / 'shaders'
  )
endforeach

# TODO: Remove this
install_data('reshade-test.fx', install_dir : datadir / 'shaders')
