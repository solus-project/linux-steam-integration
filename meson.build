project(
    'linux-steam-integration',
    ['c'],
    version: '0.5',
    license: [
        'LGPL-2.1',
    ],
    default_options: [
        'c_std=c11',
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
localedir = join_paths(prefix, get_option('localedir'))
cdata.set_quoted('SYSTEMCONFDIR', sysconfdir)
cdata.set_quoted('VENDORDIR', vendordir)
cdata.set_quoted('LOCALEDIR', localedir)
cdata.set_quoted('GETTEXT_PACKAGE', meson.project_name())

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

with_libredirect = get_option('with-libredirect')
if with_libredirect == true
    cdata.set('HAVE_LIBREDIRECT', '1')
endif

with_frontend = get_option('with-frontend')

if get_option('with-new-libcxx-abi') == true
    cdata.set('LSI_USE_NEW_ABI', '1')
endif

with_libressl_mode = get_option('with-libressl-mode')
if with_libressl_mode == 'native'
    cdata.set('LSI_LIBRESSL_MODE_NATIVE', '1')
    cdata.set('LSI_OVERRIDE_LIBRESSL', '1')
elif with_libressl_mode == 'shim'
    cdata.set('LSI_LIBRESSL_MODE_SHIM', '1')
    cdata.set('LSI_OVERRIDE_LIBRESSL', '1')
    cdata.set_quoted('LSI_LIBRESSL_SUFFIX', get_option('with-libressl-suffix'))
else
    message('LibreSSL mode isn\'t configured which may impact security')
endif

config_h = configure_file(
     configuration: cdata,
     output: 'config.h',
)
config_h_dir = include_directories('.')

subdir('data')
subdir('src')
subdir('po')
