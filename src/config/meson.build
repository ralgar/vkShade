libconfig_source = [
  'config_store.cpp',
]

libconfig_library = static_library(
  'config',
  libconfig_source,
  dependencies : [
    glm,
    inih,
    spdlog,
  ],
  include_directories : vkShade_include_path
)

libconfig = declare_dependency(
  link_with : libconfig_library,
)
