appsdir = join_paths(datadir, 'applications')

# If frontend is enabled, install lsi-settings.desktop
if with_frontend == true
    data_conf = configuration_data()
    if with_snap_support == true
        data_conf.set('LSI_SETTINGS', 'linux-steam-integration.exec $SNAP/bin/linux-steam-integration.settings')
    else
        data_conf.set('LSI_SETTINGS', 'lsi-settings')
    endif
    configure_file(
        input: 'lsi-settings.desktop.in',
        output: 'lsi-settings.desktop',
        configuration: data_conf,
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
