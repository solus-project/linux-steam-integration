#!/bin/bash
set -e
env -i DBUS_SESSION_ADDRESS=$DBUS_SESSION_ADDRESS \
       PATH=$PATH \
       DISPLAY=$DISPLAY \
       HOME=$HOME \
       USER=$USER \
       USERNAME=$USERNAME \
       LD_AUDIT="`pwd`/buildroot/install/usr/\$LIB/liblsi-intercept.so" \
       LD_PRELOAD="/usr/\$LIB/libxcb.so.1:/usr/\$LIB/libX11.so.6:/usr/\$LIB/libstdc++.so.6" \
       DBUS_FATAL_WARNINGS=0 \
       STEAM_RUNTIME=0 /usr/lib64/steam/steam
