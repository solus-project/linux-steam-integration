# Only build libintercept if told to!

if with_libintercept == true

    intercept_sources = [
        'main.c',
    ]

    sym_map = join_paths(meson.current_source_dir(), 'sym.map')

    main_intercept = shared_library(
        'lsi-intercept',
        sources: intercept_sources,
        c_args: ['-fvisibility=hidden'],
        link_args: ['-Wl,--version-script=@0@'.format(sym_map)],
        include_directories: nica_includes,
        dependencies: [
            link_lsi_common,
        ],
        install: true,
    )
endif
