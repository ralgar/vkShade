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

gpu_timing_tests = executable('gpu_timing_tests',
  'gpu_timing_tests.cpp',
  dependencies : [
    catch2,
  ],
  include_directories : include_directories('../src')
)

test('ConfigObserver', config_observer_tests)
test('ConfigParser', config_parser_tests)
test('ConfigStore', config_store_tests)
test('EventBus', event_bus_tests)
test('ReshadeUniforms', reshade_uniform_tests)
test('GpuTiming', gpu_timing_tests)
