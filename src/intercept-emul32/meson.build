intercept_sources = [
    '../intercept/main.c',
]

prefix = get_option('prefix')
emul32_libdir = join_paths(prefix, 'lib32')

emul32_intercept = shared_library(
    'lsi-intercept-32',
    sources: intercept_sources + nica_sources,
    include_directories: nica_includes,
    install: true,
    install_dir: emul32_libdir,
    c_args: ['-m32'],
    link_args: ['-m32'],
)
