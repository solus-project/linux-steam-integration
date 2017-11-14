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

    shim = executable(
        shim_target_name,
        sources: shim_sources + nica_sources,
        include_directories: nica_includes,
        dependencies: [
            link_lsi,
        ],
        install: true,
    )
endif
