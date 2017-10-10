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
