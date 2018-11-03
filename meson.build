project('bitarray', 'cpp')

add_project_arguments(
  '-mbmi2', '-mpopcnt',
  '-Werror', '-Wall', '-Wextra', '-Wpedantic',
  '-fsanitize=implicit-conversion', '-fsanitize=integer',
  '-std=c++17',
  language: 'cpp',
)

test('bitarray-test', executable('bitarray-test',
  'bitarray-test.cpp'
))
