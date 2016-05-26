EXTRA_DIST = \
	LICENSE.LGPL2.1 \
	README.md

AM_CFLAGS = \
	-fstack-protector -Wall -pedantic \
	-Wstrict-prototypes -Wundef -fno-common \
	-Werror-implicit-function-declaration \
	-Wformat -Wformat-security -Werror=format-security \
	-Wno-conversion -Wunused-variable -Wunreachable-code \
	-Wall -W -D_FORTIFY_SOURCE=2 -std=c99

noinst_LTLIBRARIES = \
	libnica.la

# Convenience library to prevent external linking with currently unstable ABI
libnica_la_SOURCES = \
	src/nica/hashmap.h \
	src/nica/hashmap.c \
	src/nica/inifile.h \
	src/nica/inifile.c \
	src/nica/util.h

bin_PROGRAMS = \
	steam

steam_SOURCES = \
	src/lsi.h \
	src/lsi.c \
	src/shim.c

steam_LDADD = \
	libnica.la
