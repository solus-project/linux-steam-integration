#!/bin/bash
set -e

if [[ ! -d buildroot/build32 ]]; then
    CC="gcc -m32" linux32 meson buildroot/build32 --prefix=/usr --sysconfdir=/etc --datadir=/usr/share --libdir=/usr/lib32
fi

if [[ ! -d buildroot/build64 ]]; then
    meson buildroot/build64 --prefix=/usr --sysconfdir=/etc --datadir=/usr/share --libdir=/usr/lib64
fi

ninja -C buildroot/build32
ninja -C buildroot/build64

DESTDIR=`pwd`/buildroot/install ninja -C buildroot/build32 install
DESTDIR=`pwd`/buildroot/install ninja -C buildroot/build64 install


