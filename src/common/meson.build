# Build the common library to share functionality between various components

lsi_common_sources = [
    'files.c',
    'log.c',
    'vdf.c',
]

lib_lsi_common = static_library(
    'lsi-lsi-common',
    sources: lsi_common_sources + nica_sources,
    c_args: ['-fvisibility=hidden'],
    include_directories: nica_includes,
)

link_lsi_common = declare_dependency(
    link_with: lib_lsi_common,
    include_directories: include_directories('.'),
)
