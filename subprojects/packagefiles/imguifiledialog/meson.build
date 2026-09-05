project('ImGuiFileDialog', 'cpp', version: '0.6.8')

sources = files('ImGuiFileDialog.cpp')

imgui = dependency('imgui-docking')

imguifiledialog_inc = include_directories('./', is_system: true)
imguifiledialog_lib = library('imguifiledialog', sources, include_directories: imguifiledialog_inc, dependencies: imgui)
imguifiledialog_dep = declare_dependency(include_directories: imguifiledialog_inc, link_with: imguifiledialog_lib, dependencies: imgui)
