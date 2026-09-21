libplatform_source = [
  'file_watcher.cpp',
  'linux/wayland_client_context.cpp',
]

libplatform_library = static_library(
  'platform',
  libplatform_source,
  dependencies : [
    spdlog,
    wayland_client,
  ],
  include_directories : vkShade_include_path
)

libplatform = declare_dependency(
  link_with : libplatform_library,
)
