hooks_source = [
  'hooks_device.cpp',
  'hooks_entrypoints.cpp',
  'hooks_instance.cpp',
  'hooks_surface.cpp',
  'hooks_swapchain.cpp',
]

vkshade = shared_library(
  meson.project_name().to_lower(),
  hooks_source,
  include_directories : vkShade_include_path,
  dependencies : [
    libconfig,
    libgui,
    libinput,
    libruntime,
    libvk,
    glm,
    imgui,
    magic_enum,
    spdlog,
    vma,
    wayland_client,
    x11,
    xcb,
    xkbcommon,
  ],
  gnu_symbol_visibility: 'hidden',
  install : true,
  install_dir : libdir
)
