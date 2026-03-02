libvk_source = [
  'buffer.cpp',
  'effect.cpp',
  'image.cpp',
  'initializers.cpp',
  'shader_module.cpp',
  'swapchain.cpp',
]

libvk_library = static_library(
  'vk',
  libvk_source,
  dependencies : [
    glm,
    imgui,
    magic_enum,
    spdlog,
    vma,
  ],
  include_directories : vkShade_include_path
)

libvk = declare_dependency(
  link_with : libvk_library,
)
