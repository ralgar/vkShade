wayland_protocols_dir = wayland_protocols.get_variable(pkgconfig: 'pkgdatadir')

wayland_protocol_names = [
  ['relative-pointer-unstable-v1', 'unstable/relative-pointer/relative-pointer-unstable-v1.xml'],
  ['pointer-constraints-unstable-v1', 'unstable/pointer-constraints/pointer-constraints-unstable-v1.xml'],
]

wayland_protocol_source = []
foreach protocol : wayland_protocol_names
  protocol_xml = wayland_protocols_dir / protocol[1]
  wayland_protocol_source += custom_target(
    protocol[0] + '-client-header',
    input: protocol_xml,
    output: protocol[0] + '-client-protocol.h',
    command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
  )
  wayland_protocol_source += custom_target(
    protocol[0] + '-private-code',
    input: protocol_xml,
    output: protocol[0] + '-protocol.c',
    command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
  )
endforeach

libinput_source = [
  'application_mouse_inhibitors.cpp',
  'input_backend_wayland.cpp',
  'input_backend_xcb.cpp',
  'input_backend_xlib.cpp',
  'input_manager.cpp',
  'mouse_capture_controller.cpp',
  'mouse_input_inhibitor_group.cpp',
  'sdl_module.cpp',
  'virtual_mouse_cursor.cpp',
  'wayland_callbacks.cpp',
  'xinput_raw_motion.cpp',
  wayland_protocol_source,
]

libinput_library = static_library(
  'input',
  libinput_source,
  dependencies : [
    glm,
    libplatform,
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
