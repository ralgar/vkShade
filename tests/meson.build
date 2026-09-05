catch2 = dependency('catch2-with-main', version : '>=3.0.0')

subdir('platform')

config_observer_tests = executable('config_observer_tests',
  'config_observer_tests.cpp',
  dependencies : [
    catch2,
    glm,
  ],
  include_directories : include_directories('../src')
)

config_parser_tests = executable('config_parser_tests',
  'config_parser_tests.cpp',
  dependencies : [
    catch2,
    glm,
  ],
  include_directories : include_directories('../src')
)

config_store_tests = executable('config_store_tests',
  'config_store_tests.cpp',
  dependencies : [
    catch2,
    glm,
    libconfig,
    spdlog,
  ],
  include_directories : include_directories('../src')
)

event_bus_tests = executable('event_bus_tests',
  'event_bus_tests.cpp',
  dependencies : [
    catch2
  ],
  include_directories : include_directories('../src/core')
)

reshade_uniform_tests = executable('reshade_uniform_tests',
  'reshade_uniform_tests.cpp',
  dependencies : [
    catch2,
  ],
  include_directories : include_directories('../src')
)

mouse_capture_controller_tests = executable('mouse_capture_controller_tests',
  'mouse_capture_controller_tests.cpp',
  '../src/input/mouse_capture_controller.cpp',
  dependencies : [
    catch2,
  ],
  include_directories : include_directories('../src')
)

virtual_mouse_cursor_tests = executable('virtual_mouse_cursor_tests',
  'virtual_mouse_cursor_tests.cpp',
  '../src/input/virtual_mouse_cursor.cpp',
  dependencies : [
    catch2,
    glm,
  ],
  include_directories : include_directories('../src')
)

xinput_raw_motion_tests = executable('xinput_raw_motion_tests',
  'xinput_raw_motion_tests.cpp',
  '../src/input/xinput_raw_motion.cpp',
  dependencies : [
    catch2,
    glm,
  ],
  include_directories : include_directories('../src')
)

mouse_input_inhibitor_group_tests = executable('mouse_input_inhibitor_group_tests',
  'mouse_input_inhibitor_group_tests.cpp',
  '../src/input/mouse_input_inhibitor_group.cpp',
  dependencies : [
    catch2,
  ],
  include_directories : include_directories('../src')
)

fake_sdl2 = shared_module('fake_sdl2',
  'fake_sdl2.cpp',
  name_prefix : '',
  install : false,
)

fake_sdl3 = shared_module('fake_sdl3',
  'fake_sdl3.cpp',
  name_prefix : '',
  install : false,
)

sdl_module_tests = executable('sdl_module_tests',
  'sdl_module_tests.cpp',
  '../src/input/application_mouse_inhibitors.cpp',
  '../src/input/mouse_input_inhibitor_group.cpp',
  '../src/input/sdl_module.cpp',
  cpp_args : [
    '-DFAKE_SDL2_PATH="' + fake_sdl2.full_path() + '"',
    '-DFAKE_SDL3_PATH="' + fake_sdl3.full_path() + '"',
  ],
  dependencies : [
    catch2,
    spdlog,
  ],
  include_directories : include_directories('../src')
)

test('ConfigObserver', config_observer_tests)
test('ConfigParser', config_parser_tests)
test('ConfigStore', config_store_tests)
test('EventBus', event_bus_tests)
test('ReshadeUniforms', reshade_uniform_tests)
test('MouseCaptureController', mouse_capture_controller_tests)
test('VirtualMouseCursor', virtual_mouse_cursor_tests)
test('XInputRawMotion', xinput_raw_motion_tests)
test('MouseInputInhibitorGroup', mouse_input_inhibitor_group_tests)
test('SdlModule', sdl_module_tests,
  depends : [
    fake_sdl2,
    fake_sdl3,
  ]
)
