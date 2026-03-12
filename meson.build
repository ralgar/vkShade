project('vkShade', ['c', 'cpp'], version: files('VERSION'), default_options: ['c_std=c11', 'cpp_std=c++23'])

vcs_tag(
    command: ['git', 'describe', '--tags', '--dirty=+dev'],
    input:  'src/version.hpp.in',
    output: 'version.hpp',
    fallback: meson.project_version()
)

cpp = meson.get_compiler('cpp')

vkShade_include_path = include_directories(['./src'])

datadir = join_paths(get_option('datadir'), 'vkShade')
libdir = get_option('libdir')

add_project_arguments('-DVK_NO_PROTOTYPES', language: 'cpp')
add_project_arguments('-DIMGUI_IMPL_VULKAN_NO_PROTOTYPES', language: 'cpp')

conf_paths = configuration_data()
conf_paths.set_quoted('SYSCONFDIR', join_paths(get_option('prefix'), get_option('sysconfdir')))
conf_paths.set_quoted('DATADIR', join_paths(get_option('prefix'), get_option('datadir')))

subdir('config')
subdir('src')

if get_option('shaders')
  subdir('shaders')
endif

if get_option('tests')
  subdir('tests')
endif
