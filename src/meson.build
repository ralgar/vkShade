version_hpp = declare_dependency(
  sources: vcs_tag(
    command: ['git', 'describe', '--tags', '--dirty=+dev'],
    input:  'version.hpp.in',
    output: 'version.hpp',
    fallback: meson.project_version()
  )
)

glm = dependency('glm', fallback: ['glm', 'glm_dep'])
imgui = dependency('imgui', fallback: ['imgui', 'imgui_dep'], static: true)
inih = dependency('inih', fallback: ['inih', 'inih_dep'], static: true)
magic_enum = dependency('magic_enum', fallback: ['magic_enum', 'magic_enum_dep'])
reshadefx = dependency('reshadefx', fallback: ['reshadefx', 'reshadefx_dep'])
spdlog = dependency('spdlog', fallback: ['spdlog', 'spdlog_dep'], static: true)
stb = dependency('stb', fallback: ['stb', 'stb_dep'])
vma = dependency('vma', fallback: ['vulkan-memory-allocator', 'vma_allocator_dep'])
wayland_client = dependency('wayland-client')
xkbcommon = dependency('xkbcommon')
xcb = dependency('xcb', required: false)
x11 = dependency('x11')

subdir('platform')
subdir('config')
subdir('gui')
subdir('input')
subdir('vk')
subdir('runtime')
subdir('hooks')
