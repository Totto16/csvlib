test_src_files = files('csv_tests.cpp', 'main.cpp')

test_deps = [csvlib_dep]

test_deps += dependency('gtest')
test_deps += dependency('gmock')

csv_tests = executable(
    'csv_tests',
    test_src_files,
    dependencies: test_deps,
    cpp_args: [
        '-D_TEST_RESOURCE_PATH='
        + meson.project_source_root() / 'csv_tests' / 'resources',
    ],
)

test('csv_tests', csv_tests)
