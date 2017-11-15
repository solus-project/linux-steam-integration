if shim_behaviour == 'replacement'
    shim_target_name = 'steam'
else
    shim_target_name = 'lsi-steam'
endif

if shim_behaviour != 'none'
    shim_sources = [
        'shim.c',
        'main.c',
    ]

    exec_sources = [
        'shim.c',
        'lsi-exec.c',
    ]

    exec_name = 'lsi-exec'
    frontend_name = 'lsi-settings'
    if with_snap_support == true
        exec_name = 'linux-steam-integration.exec'
    endif

    shim = executable(
        shim_target_name,
        sources: shim_sources + nica_sources,
        include_directories: nica_includes,
        dependencies: [
            link_lsi,
        ],
        install: true,
    )

    lsi_exec = executable(
        exec_name,
        sources: exec_sources + nica_sources,
        include_directories: nica_includes,
        dependencies: [
            link_lsi,
        ],
        install: true,
    )
endif
