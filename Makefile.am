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

AM_CPPFLAGS = \
        -I $(top_srcdir)/src \
        -I $(top_srcdir)/src/nica \
	-DSYSTEMCONFDIR=\"$(sysconfdir)\" \
	-DVENDORDIR=\"$(datadir)/defaults/linux-steam-integration\"


noinst_LTLIBRARIES = \
	libnica.la \
	liblsi.la

# Convenience library to prevent external linking with currently unstable ABI
libnica_la_SOURCES = \
	src/nica/hashmap.h \
	src/nica/hashmap.c \
	src/nica/inifile.h \
	src/nica/inifile.c \
	src/nica/util.h

libnica_la_CFLAGS = \
	$(AM_CFLAGS)


# Convenience library for shared LSI functions
liblsi_la_SOURCES = \
	src/shim/lsi.h \
	src/shim/lsi.c

liblsi_la_CFLAGS = \
	$(AM_CFLAGS)

liblsi_la_LIBADD = \
	libnica.la

bin_PROGRAMS = \
	steam

steam_SOURCES = \
	src/shim/shim.c

steam_LDADD = \
	libnica.la \
	liblsi.la

steam_CFLAGS = \
	$(AM_CFLAGS)
