libgui_source = [
  'panels/buffer_panel.cpp',
  'panels/effects_panel.cpp',
  'panels/log_panel.cpp',
  'windows/about_window.cpp',
  'windows/diagnostics_overlay.cpp',
  'windows/main_window.cpp',
  'gui_manager.cpp',
]

libgui_library = static_library(
  'gui',
  libgui_source,
  dependencies : [
    glm,
    imgui,
    magic_enum,
    spdlog,
    version_hpp,
    vma,
  ],
  include_directories : vkShade_include_path
)

libgui = declare_dependency(
  link_with : libgui_library,
)
