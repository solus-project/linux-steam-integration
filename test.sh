#!/bin/bash
set -e
unset XMODIFIERS
LD_AUDIT="`pwd`/buildroot/install/usr/\$LIB/liblsi-intercept.so" STEAM_RUNTIME=0 /usr/lib64/steam/steam
