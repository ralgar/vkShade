project('vkShade', ['c', 'cpp'], default_options: ['c_std=c11', 'cpp_std=c++23'])

cpp = meson.get_compiler('cpp')

vkShade_include_path = include_directories(['./src'])

datadir = join_paths(get_option('datadir'), 'vkShade')
libdir = get_option('libdir')

add_project_arguments('-DVK_NO_PROTOTYPES', language: 'cpp')
add_project_arguments('-DIMGUI_IMPL_VULKAN_NO_PROTOTYPES', language: 'cpp')

# Dependencies
glm = dependency('glm', fallback: ['glm', 'glm_dep'])
imgui = dependency('imgui', fallback: ['imgui', 'imgui_dep'], static: true)
magic_enum = dependency('magic_enum', fallback: ['magic_enum', 'magic_enum_dep'])
spdlog = dependency('spdlog', fallback: ['spdlog', 'spdlog_dep'])
vma = dependency('vma', fallback: ['vulkan-memory-allocator', 'vma_allocator_dep'])
wayland_client = dependency('wayland-client')
xkbcommon = dependency('xkbcommon')
xcb = dependency('xcb', required: false)
x11 = dependency('x11')

subdir('shaders')

if get_option('with_so')
  subdir('src')
endif

if get_option('with_json')
  subdir('config')
endif

if get_option('tests')
  subdir('tests')
endif
