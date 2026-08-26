libplatform_source = [
  'clipboard.cpp',
  'file_watcher.cpp',
  'linux/xcb_clipboard.cpp',
]

libplatform_library = static_library(
  'platform',
  libplatform_source,
  dependencies : [
    spdlog,
    xcb,
  ],
  include_directories : vkShade_include_path
)

libplatform = declare_dependency(
  link_with : libplatform_library,
)
