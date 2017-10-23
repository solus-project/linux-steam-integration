# Only build libredirect if told to!

if with_libredirect == true

    # Might not need libdl for alt libcs
    libdl = meson.get_compiler('c').find_library('dl', required : false)

    redirect_sources = [
        'main.c',
        'profile.c',
        'profiles/ark.c',
        'profiles/project_highrise.c',
    ]

    sym_map = join_paths(meson.current_source_dir(), 'sym.map')

    main_redirect = shared_library(
        'lsi-redirect',
        sources: redirect_sources,
        c_args: ['-fvisibility=hidden'],
        link_args: ['-Wl,--version-script=@0@'.format(sym_map)],
        include_directories: nica_includes,
        dependencies: [
            libdl,
            link_lsi_common,
        ],
        install: true,
    )
endif
