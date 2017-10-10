project(
    'linux-steam-integration',
    ['c'],
    version: '0.3',
    license: [
        'LGPL-2.1',
    ],
    default_options: [
        'c_std=c99',
    ],
)


am_cflags = [
    '-fstack-protector',
    '-pedantic',
    '-Wstrict-prototypes',
    '-Wundef',
    '-fno-common',
    '-Werror-implicit-function-declaration',
    '-Wformat',
    '-Wformat-security',
    '-Werror=format-security',
    '-Wno-conversion',
    '-Wunused-variable',
    '-Wunreachable-code',
    '-Wall',
    '-W',
    '-D_FORTIFY_SOURCE=2',
]

add_global_arguments(am_cflags, language: 'c')

dep_gtk3 = dependency('gtk+-3.0', version: '>= 3.4.0')

cdata = configuration_data()

cdata.set_quoted('PACKAGE_NAME', meson.project_name())

prefix = get_option('prefix')
datadir = join_paths(prefix, get_option('datadir'))
vendordir = join_paths(datadir, 'defaults', meson.project_name())
sysconfdir = join_paths(prefix, get_option('sysconfdir'))
cdata.set_quoted('SYSTEMCONFDIR', sysconfdir)
cdata.set_quoted('VENDORDIR', vendordir)

# TODO: Make this come from the meson_options.txt
cdata.set_quoted('LSI_PRELOAD_LIBS', '/usr/\$LIB/libxcb.so.1:/usr/\$LIB/libX11.so.6:/usr/\$LIB/libstdc++.so.6')
cdata.set_quoted('STEAM_BINARY', '/usr/lib64/steam/steam')
cdata.set('REPLACE_STEAM', '1')

config_h = configure_file(
     configuration: cdata,
     output: 'config.h',
)
config_h_dir = include_directories('.')

subdir('src')
