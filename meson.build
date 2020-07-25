project('bitarray', 'cpp',
  default_options: [
    'buildtype=debugoptimized',
    'warning_level=3',
    'werror=true',
    'cpp_std=c++17',
    'b_sanitize=address,undefined',
    'b_lundef=false',
  ]
)

add_project_arguments(
  '-mbmi2',
  language: 'cpp',
)

gtest = subproject('gtest').get_variable('gtest_main_dep')
test('bitarray-test', executable('bitarray-test', 'bitarray-test.cc', dependencies: [gtest]))
