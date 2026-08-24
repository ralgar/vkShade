libplatform_source = [
  'file_watcher.cpp',
]

libplatform_library = static_library(
  'platform',
  libplatform_source,
  dependencies : [
    spdlog,
  ],
  include_directories : vkShade_include_path
)

libplatform = declare_dependency(
  link_with : libplatform_library,
)
