project('ImGuiFileDialog', 'cpp', version: '0.6.8')

sources = files('ImGuiFileDialog.cpp')

imgui = dependency('imgui-docking')

imguifiledialog_inc = include_directories('./', is_system: true)

cpp_args = []
custom_config = get_option('custom_config')
if custom_config != ''
  cpp_args += '-DCUSTOM_IMGUIFILEDIALOG_CONFIG="' + custom_config + '"'
endif

imguifiledialog_lib = library('imguifiledialog', sources,
  include_directories: imguifiledialog_inc,
  dependencies: imgui,
  cpp_args: cpp_args,
)

imguifiledialog_dep = declare_dependency(include_directories: imguifiledialog_inc, link_with: imguifiledialog_lib, dependencies: imgui)
