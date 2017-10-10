#!/bin/bash

linux32 meson build32 --prefix=/usr --sysconfdir=/etc --datadir=/usr/share --libdir=/usr/lib32 CC="gcc -m32"
meson build64 --prefix=/usr --sysconfdir=/etc --datadir=/usr/share --libdir=/usr/lib64

ninja -C build32
ninja -C build64
