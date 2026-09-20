linux_filewatcher_tests = executable('linux_filewatcher_tests',
  'file_watcher_tests.cpp',
  dependencies : [
    catch2,
    spdlog,
  ],
  include_directories : vkShade_include_path
)

test('LinuxFileWatcher', linux_filewatcher_tests)

wayland_client_context_tests = executable('wayland_client_context_tests',
  'wayland_client_context_tests.cpp',
  '../../..' / 'src/platform/linux/wayland_client_context.cpp',
  dependencies : [
    catch2,
    spdlog,
    wayland_client,
  ],
  include_directories : vkShade_include_path
)

test('WaylandClientContext', wayland_client_context_tests)
