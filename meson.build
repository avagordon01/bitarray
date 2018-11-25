project('bitarray', 'cpp')

add_project_arguments(
  '-mbmi2', '-mpopcnt',
  '-Werror', '-Wall', '-Wextra', '-Wpedantic',
  '-fsanitize=implicit-conversion', '-fsanitize=integer', '-fsanitize=undefined',
  '-std=c++17',
  language: 'cpp',
)

executable('bitarray-test', 'bitarray-test.cpp')
executable('divide-test', 'divide-test.cpp')
