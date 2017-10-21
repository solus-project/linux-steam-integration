# Only build libintercept if told to!

if with_libintercept == true

    intercept_sources = [
        'main.c',
    ]

    main_intercept = shared_library(
        'lsi-intercept',
        sources: intercept_sources,
        c_args: ['-fvisibility=hidden'],
        include_directories: nica_includes,
        dependencies: [
            link_lsi_common,
        ],
        install: true,
    )
endif
