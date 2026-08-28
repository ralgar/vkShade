libruntime_source = [
  'reshade_effect.cpp',
  'reshade_uniforms.cpp',
  'runtime.cpp',
]

libruntime_library = static_library(
  'runtime',
  libruntime_source,
  dependencies : [
    glm,
    imgui,
    magic_enum,
    reshadefx,
    spdlog,
    stb,
    vma,
  ],
  include_directories : vkShade_include_path
)

libruntime = declare_dependency(
  link_with : libruntime_library,
)
