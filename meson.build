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

# Configurables
cdata.set_quoted('LSI_PRELOAD_LIBS', get_option('with-preload-libs'))
cdata.set_quoted('STEAM_BINARY', get_option('with-steam-binary'))

shim_behaviour = get_option('with-shim')
if shim_behaviour == 'replacement'
    cdata.set('REPLACE_STEAM', '1')
endif

with_libintercept = get_option('with-libintercept')
if with_libintercept == true
    cdata.set('HAVE_LIBINTERCEPT', '1')
endif

with_frontend = get_option('with-frontend')

config_h = configure_file(
     configuration: cdata,
     output: 'config.h',
)
config_h_dir = include_directories('.')

subdir('src')
