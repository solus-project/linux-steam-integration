# Track how to build nica, as we actually build it twice..

nica_sources = [
    'libnica/src/array.c',
    'libnica/src/hashmap.c',
    'libnica/src/files.c',
    'libnica/src/inifile.c',
    'libnica/src/list.c',
    'libnica/src/nc-string.c',
    'libnica/src/util.c',
]

nica_includes = [
    include_directories('libnica/src/include'),
    include_directories('libnica/src/include/nica'),
]

main_nica = static_library(
    'nica',
    sources: [
        nica_sources,
    ],
    include_directories: nica_includes,
    link_args: ['-export-dynamic'],
)

emul32_nica = static_library(
    'nica-emul32',
    sources: [
        nica_sources,
    ],
    include_directories: nica_includes,
    link_args: ['-export-dynamic'],
    c_args: ['-m32'],
)

link_main_nica = declare_dependency(
    link_with: main_nica,
    include_directories: nica_includes,
)

link_emul32_nica = declare_dependency(
    link_with: emul32_nica,
    include_directories: nica_includes,
)

subdir('intercept')
subdir('intercept-emul32')
