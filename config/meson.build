data_dir = get_option('datadir')

vulkan_layer_dir = join_paths(data_dir, 'vulkan', 'implicit_layer.d')

configure_file(
    input : 'vkShade.json.in',
    output : 'vkShade.json',
    configuration : {'ld_lib_dir_vkshade' : ld_lib_dir_vkshade},
    install : true,
    install_dir : vulkan_layer_dir,
)
