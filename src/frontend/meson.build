if get_option('with-frontend') == true
    dep_gtk3 = dependency('gtk+-3.0', version: '>= 3.4.0')

    frontend_sources = [
        'main.c',
    ]

    frontend = executable(
        'lsi-settings',
        sources: frontend_sources + nica_sources,
        include_directories: nica_includes,
        dependencies: [
            dep_gtk3,
        ],
        install: true,
    )
endif
