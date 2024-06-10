project(
    'csvlib',
    'cpp',
    version: '1.0.0',
    license: 'MIT',
    meson_version: '>=1.2.0',
    default_options: {
        'buildtype': 'release',
        'optimization': '3',
        'cpp_std': 'c++23',
        'warning_level': '3',
        'werror': 'false',
        'b_ndebug': 'if-release',
    },
)

inc_dirs = []

subdir('csvlib')


csvlib_dep = declare_dependency(
    include_directories: inc_dirs,
    version: meson.project_version(),
    link_with: csvlib_lib,
)
