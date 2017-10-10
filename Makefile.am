EXTRA_DIST = \
	LICENSE.LGPL2.1 \
	README.rst \
	data/lsi-settings.desktop \
	data/lsi-steam.desktop

AM_CFLAGS = \
	-fstack-protector -Wall -pedantic \
	-Wstrict-prototypes -Wundef -fno-common \
	-Werror-implicit-function-declaration \
	-Wformat -Wformat-security -Werror=format-security \
	-Wno-conversion -Wunused-variable -Wunreachable-code \
	-Wall -W -D_FORTIFY_SOURCE=2 -std=c99

AM_CPPFLAGS = \
	-I $(top_srcdir)/src \
	-I $(top_srcdir)/src/shim \
	-I $(top_srcdir)/src/libnica/src/include \
	-DSYSTEMCONFDIR=\"$(sysconfdir)\" \
	-DVENDORDIR=\"$(datadir)/defaults/linux-steam-integration\"

SUBDIRS = src/libnica

noinst_LTLIBRARIES = \
	liblsi.la

lib_LTLIBRARIES = \
	liblsi-intercept.la

bin_PROGRAMS = 
desktopdir = $(datadir)/applications
desktop_DATA =

if HAVE_FRONTEND
bin_PROGRAMS += lsi-settings

lsi_settings_SOURCES = \
	src/frontend/main.c

lsi_settings_CFLAGS = \
	$(GTK_CFLAGS) \
	$(AM_CFLAGS) \
	-DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_3_4 \
	-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_4

lsi_settings_LDADD = \
	liblsi.la \
	$(GTK_LIBS)

desktopdir = $(datadir)/applications
desktop_DATA += \
	data/lsi-settings.desktop

endif

# Convenience library for shared LSI functions
liblsi_la_SOURCES = \
	src/shim/lsi.h \
	src/shim/lsi.c

liblsi_la_CFLAGS = \
	$(AM_CFLAGS) \
	-I$(top_srcdir)/src/libnica/src/include/nica

liblsi_la_LIBADD = \
	src/libnica/libnica.la

# Replace the main steam binary with our own steam file
if REPLACE_STEAM

bin_PROGRAMS += steam

steam_SOURCES = \
	src/shim/shim.c

steam_LDADD = \
	src/libnica/libnica.la \
	liblsi.la

steam_CFLAGS = \
	$(AM_CFLAGS)

else
# Don't replace steam - use a non conflicting filename

bin_PROGRAMS += lsi-steam

lsi_steam_SOURCES = \
	src/shim/shim.c

lsi_steam_LDADD = \
	src/libnica/libnica.la \
	liblsi.la

lsi_steam_CFLAGS = \
	$(AM_CFLAGS)

# Install custom .desktop file for steam
desktop_DATA += \
	data/lsi-steam.desktop

endif

# TODO: If enable intercept
liblsi_intercept_la_SOURCES = \
	src/intercept/main.c

liblsi_intercept_la_CFLAGS = \
	$(AM_CFLAGS) \
	-I$(top_srcdir)/src/libnica/src/include/nica

liblsi_intercept_la_LIBADD = \
	src/libnica/libnica.la

liblsi_intercept_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-fvisibility=hidden \
	-module \
	-avoid-version
