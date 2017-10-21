# Only build libredirect if told to!

if with_libredirect == true

    # Might not need libdl for alt libcs
    libdl = meson.get_compiler('c').find_library('dl', required : false)

    redirect_sources = [
        'main.c',
    ]

    main_redirect = shared_library(
        'lsi-redirect',
        sources: redirect_sources,
        c_args: ['-fvisibility=hidden'],
        include_directories: nica_includes,
        dependencies: [
            libdl,
            link_lsi_common,
        ],
        install: true,
    )
endif
