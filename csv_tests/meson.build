test_src_files = files('csv_tests.cpp', 'main.cpp')

test_deps = [csvlib_dep]

test_deps += dependency('gtest')
test_deps += dependency('gmock')

csv_tests = executable(
    'csv_tests',
    test_src_files,
    dependencies: test_deps,
)

subdir('resources')

test('csv_tests', csv_tests)
