config_globals_hpp = configure_file(
  output: 'config_globals.hpp',
  configuration: conf_paths
)

libconfig_source = [
  config_globals_hpp,
  'config_manager.cpp',
  'config_store.cpp',
]

libconfig_library = static_library(
  'config',
  libconfig_source,
  dependencies : [
    glm,
    inih,
    libplatform,
    magic_enum,
    spdlog,
  ],
  include_directories : vkShade_include_path
)

libconfig = declare_dependency(
  link_with : libconfig_library,
)
