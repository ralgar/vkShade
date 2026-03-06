catch2 = dependency('catch2-with-main', version : '>=3.0.0')

config_parser_tests = executable('config_parser_tests',
  'config_parser_tests.cpp',
  dependencies : [
    catch2,
    glm,
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

test('ConfigParser', config_parser_tests)
test('EventBus', event_bus_tests)
