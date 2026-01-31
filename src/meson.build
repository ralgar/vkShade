vkShade_src = [
  'gui/gui_manager.cpp',
  'hooks/hooks_device.cpp',
  'hooks/hooks_entrypoints.cpp',
  'hooks/hooks_instance.cpp',
  'hooks/hooks_surface.cpp',
  'hooks/hooks_swapchain.cpp',
  'input/input_manager.cpp',
  'input/input_manager_wayland.cpp',
  'input/input_manager_xcb.cpp',
  'input/input_manager_xlib.cpp',
  'input/wayland_callbacks.cpp',
  'vk/buffer.cpp',
  'vk/effect.cpp',
  'vk/image.cpp',
  'vk/shader_module.cpp',
  'vk/swapchain.cpp',
]

conf_paths = configuration_data()
conf_paths.set_quoted('SYSCONFDIR', join_paths(get_option('prefix'), get_option('sysconfdir')))
conf_paths.set_quoted('DATADIR', join_paths(get_option('prefix'), get_option('datadir')))

shared_library(
  meson.project_name().to_lower(),
  vkShade_src,
  include_directories : vkShade_include_path,
  dependencies : [
    imgui,
    magic_enum,
    spdlog,
    xcb,
    x11,
    xkbcommon,
    vma,
    wayland_client,
  ],
  gnu_symbol_visibility: 'hidden',
  install : true,
  install_dir : lib_dir
)
