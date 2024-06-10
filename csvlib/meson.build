inc_dirs += include_directories('.')

compile_args = []


src_files = files(
    'csv/datasource/utf8/DataSource.cpp',
    'csv/parser.cpp',
)

deps = []

icu_dep = dependency('icu', required: false)

if icu_dep.found()
    deps += icu_dep
    src_files += files('csv/datasource/icu/DataSource.cpp')
    compile_args += '-DALLOW_ICU_EXTENSIONS'
endif

csvlib_lib = library(
    'csvlib',
    src_files,
    cpp_args: compile_args,
    dependencies: deps,
    include_directories: inc_dirs,
)
