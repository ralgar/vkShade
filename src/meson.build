vkShade_src = [
  'input/input_manager.cpp',
  'input/input_manager_wayland.cpp',
  'input/wayland_callbacks.cpp',
  'layer.cpp',
  'vulkan_hooks_device.cpp',
  'vulkan_hooks_instance.cpp',
  'vulkan_hooks_surface.cpp',
  'vulkan_hooks_swapchain.cpp',
  'vulkan_shader_module.cpp',
  'vulkan_swapchain.cpp',
]

conf_paths = configuration_data()
conf_paths.set_quoted('SYSCONFDIR', join_paths(get_option('prefix'), get_option('sysconfdir')))
conf_paths.set_quoted('DATADIR', join_paths(get_option('prefix'), get_option('datadir')))

shared_library(
  meson.project_name().to_lower(),
  vkShade_src,
  include_directories : vkShade_include_path,
  dependencies : [
    magic_enum,
    spdlog,
    xkbcommon,
    vma,
    wayland_client,
  ],
  gnu_symbol_visibility: 'hidden',
  install : true,
  install_dir : lib_dir
)
