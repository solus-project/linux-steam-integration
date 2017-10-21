# Track how to build nica, as we actually build it twice..

nica_sources = [
    '../libnica/src/array.c',
    '../libnica/src/hashmap.c',
    '../libnica/src/files.c',
    '../libnica/src/inifile.c',
    '../libnica/src/list.c',
    '../libnica/src/nc-string.c',
    '../libnica/src/util.c',
]

nica_includes = [
    include_directories('libnica/src/include'),
    include_directories('libnica/src/include/nica'),
    config_h_dir,
]

subdir('common')
subdir('lsi')
subdir('frontend')
subdir('intercept')
subdir('redirect')
subdir('shim')
