#!/bin/bash
set -e
unset XMODIFIERS
export DBUS_FATAL_WARNINGS=1
LD_AUDIT="`pwd`/buildroot/install/usr/\$LIB/liblsi-intercept.so" STEAM_RUNTIME=0 /usr/lib64/steam/steam
