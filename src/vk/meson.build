libvk_source = [
  'buffer.cpp',
  'gpu_timing.cpp',
  'image.cpp',
  'image_tracker.cpp',
  'initializers.cpp',
  'sampler.cpp',
  'shader_module.cpp',
]

libvk_library = static_library(
  'vk',
  libvk_source,
  dependencies : [
    glm,
    magic_enum,
    reshadefx,
    spdlog,
    stb,
    vma,
  ],
  include_directories : vkShade_include_path
)

libvk = declare_dependency(
  link_with : libvk_library,
)
