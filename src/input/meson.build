libinput_source = [
  'input_backend_wayland.cpp',
  'input_backend_xcb.cpp',
  'input_backend_xlib.cpp',
  'input_manager.cpp',
  'wayland_callbacks.cpp',
]

libinput_library = static_library(
  'input',
  libinput_source,
  dependencies : [
    glm,
    spdlog,
    xcb,
    x11,
    xkbcommon,
    wayland_client,
  ],
  include_directories : vkShade_include_path
)

libinput = declare_dependency(
  link_with : libinput_library,
)
