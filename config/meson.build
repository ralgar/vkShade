data_dir = get_option('datadir')

vulkan_layer_dir = join_paths(get_option('datadir'), 'vulkan', 'implicit_layer.d')
libdir_absolute = join_paths(get_option('prefix'), libdir) + '/'

configure_file(
    input : 'vkShade.json.in',
    output : 'vkShade.json',
    configuration : {'ld_lib_dir_vkshade' : libdir_absolute},
    install : true,
    install_dir : vulkan_layer_dir,
)
