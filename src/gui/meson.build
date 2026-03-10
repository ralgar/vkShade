libgui_source = [
  'gui_manager.cpp',
  'shader_manager.cpp',
]

libgui_library = static_library(
  'gui',
  libgui_source,
  dependencies : [
    glm,
    imgui,
    magic_enum,
    spdlog,
    vma,
  ],
  include_directories : vkShade_include_path
)

libgui = declare_dependency(
  link_with : libgui_library,
)
