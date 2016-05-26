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

bin_PROGRAMS = \
	steam

steam_SOURCES = \
	src/lsi.h \
	src/lsi.c \
	src/shim.c
