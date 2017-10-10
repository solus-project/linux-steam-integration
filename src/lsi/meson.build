# Only build liblsi if told to!

if shim_behaviour != 'none' or with_frontend == true

    lsi_sources = [
        'lsi.c',
    ]

    lib_lsi = static_library(
        'lsi-lsi',
        sources: lsi_sources + nica_sources,
        include_directories: nica_includes,
    )
    link_lsi = declare_dependency(
        link_with: lib_lsi,
        include_directories: include_directories('.'),
    )
endif
