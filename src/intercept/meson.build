# Only build libintercept if told to!

if with_libintercept == true

    intercept_sources = [
        'main.c',
    ]

    nica_build = static_library(
        'nica',
        sources: nica_sources,
        include_directories: nica_includes,
    )

    link_nica = declare_dependency(
        link_with: nica_build,
        include_directories: nica_includes,
    )


    main_intercept = shared_library(
        'lsi-intercept',
        sources: intercept_sources,
        c_args: ['-fvisibility=hidden'],
        include_directories: nica_includes,
        dependencies: [
            link_nica,
        ],
        install: true,
    )
endif
