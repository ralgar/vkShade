catch2_dep = dependency('catch2-with-main', version : '>=3.0.0')

event_bus_test = executable('event_bus_tests',
  'event_bus_tests.cpp',
  dependencies : [catch2_dep],
  include_directories : include_directories('../src/core')
)

test('EventBus', event_bus_test)
