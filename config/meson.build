data_dir = get_option('datadir')

vulkan_layer_dir = join_paths(get_option('datadir'), 'vulkan', 'implicit_layer.d')
libdir_absolute = join_paths(get_option('prefix'), libdir) + '/'

is_32bit = cpp.sizeof('void*') == 4
arch_suffix = is_32bit ? '_32' : '_64'

configure_file(
    input : 'vkShade.json.in',
    output : is_32bit ? 'vkShade_i386.json' : 'vkShade_x86_64.json',
    configuration : {
      'ld_lib_dir_vkshade' : libdir_absolute,
      'arch_suffix' : arch_suffix
    },
    install : true,
    install_dir : vulkan_layer_dir,
)

install_data('vkShade.ini', install_dir: get_option('datadir') / 'vkShade')
install_data('ReShade.ini', install_dir: get_option('datadir') / 'vkShade')
