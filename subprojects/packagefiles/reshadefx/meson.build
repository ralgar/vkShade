project('reshade', 'cpp',
  version : '6.x',
  default_options : [
    'warning_level=1',
    'cpp_std=c++17',
  ],
)

reshadefx_sources = files(
  'source/effect_codegen_glsl.cpp',
  'source/effect_codegen_hlsl.cpp',
  'source/effect_codegen_spirv.cpp',
  'source/effect_expression.cpp',
  'source/effect_lexer.cpp',
  'source/effect_parser_exp.cpp',
  'source/effect_parser_stmt.cpp',
  'source/effect_preprocessor.cpp',
  'source/effect_symbol_table.cpp',
)

reshadefx_inc = include_directories('source', is_system: true)

spirv_headers_dep = subproject('spirv-headers').get_variable('spirv_headers_dep')

reshadefx_lib = static_library('reshadefx',
  reshadefx_sources,
  dependencies : spirv_headers_dep,
  include_directories : reshadefx_inc,
  cpp_args : ['-DNDEBUG', '-w'],
  install : false,
)

reshadefx_dep = declare_dependency(
  link_with : reshadefx_lib,
  include_directories : reshadefx_inc,
)
