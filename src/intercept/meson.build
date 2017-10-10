# Only build libintercept if told to!

if with_libintercept == true

    intercept_sources = [
        'main.c',
    ]

    main_intercept = shared_library(
        'lsi-intercept',
        sources: intercept_sources + nica_sources,
        include_directories: nica_includes,
        install: true,
    )
endif
