# Only build libintercept if told to!

if with_libredirect == true

    redirect_sources = [
        'main.c',
    ]

    main_redirect = shared_library(
        'lsi-redirect',
        sources: redirect_sources,
        c_args: ['-fvisibility=hidden'],
        include_directories: nica_includes,
        install: true,
    )
endif
