libinput_source = [
  'application_mouse_inhibitors.cpp',
  'input_backend_wayland.cpp',
  'input_backend_xcb.cpp',
  'input_backend_xlib.cpp',
  'input_manager.cpp',
  'mouse_capture_controller.cpp',
  'mouse_input_inhibitor_group.cpp',
  'virtual_mouse_cursor.cpp',
  'wayland_callbacks.cpp',
]

libinput_library = static_library(
  'input',
  libinput_source,
  dependencies : [
    glm,
    magic_enum,
    spdlog,
    wayland_client,
    x11,
    xcb,
    xi,
    xkbcommon,
  ],
  include_directories : vkShade_include_path
)

libinput = declare_dependency(
  link_with : libinput_library,
)
