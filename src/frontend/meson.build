if with_frontend == true
    dep_gtk3 = dependency('gtk+-3.0', version: '>= 3.4.0')

    frontend_sources = [
        'main.c',
        'main-window.c',
    ]

    frontend_name = 'lsi-settings'
    if with_snap_support == true
        frontend_name = 'linux-steam-integration.settings'
    endif

    frontend = executable(
        frontend_name,
        sources: frontend_sources + nica_sources,
        include_directories: nica_includes,
        dependencies: [
            dep_gtk3,
            link_lsi,
        ],
        install: true,
    )
endif
