appsdir = join_paths(datadir, 'applications')

# If frontend is enabled, install lsi-settings.desktop
if with_frontend == true
    install_data(
        'lsi-settings.desktop',
        install_dir: appsdir,
    )
endif

# If we need to co-exist, install alternative .desktop file (not conflicting with Steam)
if shim_behaviour == 'co-exist'
    install_data(
        'lsi-steam.desktop',
        install_dir: appsdir,
    )
endif
