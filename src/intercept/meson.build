intercept_sources = [
    'main.c',
]

main_intercept = shared_library(
    'lsi-intercept',
    sources: intercept_sources,
    include_directories: config_h_dir,
    dependencies: link_main_nica,
    install: true,
)

prefix = get_option('prefix')
emul32_libdir = join_paths(prefix, 'lib32')

emul32_intercept = shared_library(
    'lsi-intercept-32',
    sources: intercept_sources,
    include_directories: config_h_dir,
    dependencies: link_emul32_nica,
    install: true,
    install_dir: emul32_libdir,
    c_args: ['-m32'],
)
