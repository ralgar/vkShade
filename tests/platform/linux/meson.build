linux_filewatcher_tests = executable('linux_filewatcher_tests',
  'file_watcher_tests.cpp',
  dependencies : [
    catch2,
    spdlog,
  ],
  include_directories : vkShade_include_path
)

test('LinuxFileWatcher', linux_filewatcher_tests)
