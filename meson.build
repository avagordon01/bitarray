project('bitarray', 'cpp',
  default_options: [
    'buildtype=debugoptimized',
    'warning_level=3',
    'cpp_std=c++20',
    'b_sanitize=address,undefined',
    'b_lundef=false',
  ]
)

add_project_arguments(
  '-mbmi2',
  language: 'cpp',
)

gtest = dependency('gtest_main')

test_args = [
  {'name': 'u128', 'args': ['-DTYPE=__uint128_t']},
  {'name': 'u64', 'args': ['-DTYPE=uint64_t']},
  {'name': 'u32', 'args': ['-DTYPE=uint32_t']},
  {'name': 'u16', 'args': ['-DTYPE=uint16_t']},
  {'name': 'u8',  'args': ['-DTYPE=uint8_t']},
]
foreach test_arg : test_args
  test('bitarray-test-' + test_arg['name'],
    executable('bitarray-test-' + test_arg['name'], 'bitarray-test.cc', dependencies: [gtest], cpp_args: [test_arg['args']])
  )
endforeach
